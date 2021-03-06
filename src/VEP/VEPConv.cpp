// VEP convolution
//
// Copyright (C) 2005-2007 Stefan Kersten <sk@k-hornz.de>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA

#include "VEPConv.h"
#include "VEPDSP.h"

#include "clz.h"

#undef NDEBUG
#include <assert.h>
#include <algorithm>
#include <limits>
#include <sndfile.h>
#include <string.h>

using namespace VEP;

// =====================================================================
// VEP::Response

VEP::Response::Response(size_t numChannels, size_t numFrames, size_t minSize, size_t maxSize)
  : m_numChannels(numChannels),
    m_numFrames(numFrames),
    m_minPartSize(minSize),
    m_maxPartSize(std::min(maxSize, (size_t)FFT::kMaxLogSize)),
    m_size(0),
    m_numPartitions(0)
{
  initModules(numFrames, minSize, maxSize);
}

void VEP::Response::printOn(FILE *stream) const
{
  fprintf(stream, "VEPResponse: modules %d size %d\n", numModules(), m_size);
  for (size_t i = 0; i < numModules(); ++i) {
    fprintf(stream, "%3d %5d|%5d  %d\n", 1<<i, m_modules[i].offset(), m_modules[i].size(), m_modules[i].count());
  }
}

void VEP::Response::initModules(size_t numFrames, size_t minSize, size_t maxSize)
{
  size_t partSize = minSize;
  size_t rest = numFrames;
  m_size = 0;
  while (rest > 0) {
    size_t maxCount = partSize >= maxSize
      ? std::numeric_limits<size_t>::max() : (numModules() == 0 ? 4 : 2);
    rest = addModule(m_size, partSize, maxCount, rest);
    partSize *= 2;
  }
}

size_t VEP::Response::addModule(size_t offset, size_t size, size_t maxCount, size_t rest)
{
  m_modules.push_back(
    Module(
      offset, size,
      std::min(maxCount, rest / size + (rest % size ? 1 : 0)),
      FFT::get(LOG2CEIL(size), true)));
  const size_t l = size * m_modules.back().count();
  m_size += l;
  m_numPartitions += m_modules.back().count();
  return rest - std::min(rest, l);
}

// =====================================================================
// Convolver

Convolver::Convolver(
  size_t numChannels,
  size_t binSize,
  const Response::Module& module,
  size_t externalDelay
  )
: m_numChannels(numChannels),
  m_binSize(binSize),
  m_module(module),
  m_stage(0),
  m_binIndex(0),
  m_inputSpecPos(0),
  m_inputBuffer(m_numChannels, partitionSize() * 4),  // [ work ] [ pad ] [ fill ] [ pad ]
  m_inputSpecBuffer(m_numChannels, numPartitions() * fft()->paddedSize()),
  m_outputBuffer(m_numChannels, irOffset() + partitionSize()),
  m_irBuffer(m_numChannels, numPartitions() * fft()->paddedSize()),
  m_fftMACBuffer(m_numChannels, fft()->paddedSize()),
  m_overlapBuffer(m_numChannels, partitionSize()),
  m_fftBuffer(m_numChannels, fft()->paddedSize())
{
//   printf("Convolver: numBins %d partitionSize %d numPartitions %d partitionOffset %d irOffset %d\n",
//       numBins(), partitionSize(), numPartitions(), m_partitionOffset, irOffset());

  assert( (irOffset() % partitionSize()) == 0 );

  // if (irOffset() > 0) {
  //   // delay input buffer by partition size (plus padding for the fft)
  //   m_inputBuffer.writeAdvance(partitionSize()*2);
  //   // pre-delay convolver output for later partitions
  //   // NOTE: input already delayed by partitionSize
  //   size_t delay = irOffset() - partitionSize();
  //   m_outputBuffer.writeAdvance(delay);
  // }
  
  m_outputBuffer.writeAdvance(irOffset() - (partitionSize() - externalDelay) * (irOffset() > 0));
}

void Convolver::pushInput(const float** src, size_t numChannels, size_t numFrames)
{
  // printf("pushInput %d wpos=%d wspace=%d size=%d iroff=%d\n", numFrames, m_inputBuffer.writePos(), m_inputBuffer.writeSpace(), m_inputBuffer.size(), irOffset());

  assert( numFrames <= binSize() );
  assert( m_inputBuffer.writeSpace() >= binSize() );
  assert( numChannels == m_numChannels );
  
  for (size_t c=0; c < numChannels; ++c)
  {
    float *dst = m_inputBuffer.writeVector(c);
    // copy input to ringbuffer
    memCopy(dst, src[c], std::min(binSize(), numFrames));
    // clear remainder
    if (numFrames < binSize()) {
      memZero(dst + numFrames, binSize() - numFrames);
    }
  }
  
  m_inputBuffer.writeAdvance(binSize());

  // clear second half of time-domain input for forward FFT
  if ((m_inputBuffer.writePos() % partitionSize()) == 0) {
    for (size_t c=0; c < numChannels; ++c)
      memZero(m_inputBuffer.writeVector(c), partitionSize());
    m_inputBuffer.writeAdvance(partitionSize());
  }

  // printf("pushInput rpos=%d rspace=%d %d\n", m_inputBuffer.readPos(), m_inputBuffer.readSpace(), m_inputBuffer.size());
}

void Convolver::pullOutput(float** channelData, size_t numChannels, size_t size)
{
//   printf("pullOutput %d %d %d %d\n", binSize(), m_outputBuffer[0].readPos(), m_outputBuffer[0].readSpace(), m_outputBuffer[0].size());

  assert( size <= binSize() );

  // NOTE: m_outputBuffer.readSpace() will return zero, because the
  // write pointer already has been advanced and equals the read
  // pointer. that's not a problem, because read and write accesses
  // only take place interleaved, i.e. by the next time data is
  // written to the buffer the space made by this routine is used.

  if (irOffset() == 0) {
    for (size_t c=0; c < numChannels; ++c)
    {
      // first partition: assign
      float* dst = channelData[c];
      float* src = m_outputBuffer.readVector(c);
      for (size_t i=0; i < size; ++i)
        *dst++ = *src++;
    }
  } else {
    for (size_t c=0; c < numChannels; ++c)
    {
      // later partition: mix
      float* dst = channelData[c];
      float* src = m_outputBuffer.readVector(c);
      for (size_t i=0; i < size; ++i)
        *dst++ += *src++;
    }
  }

  m_outputBuffer.readAdvance(binSize());
}

void Convolver::compute(size_t binIndex)
{
  // schedule process if
  //   numStages := 2
  //   period := numBins/numStages
  //   ((binIndex - period/2) % period) = 0
  if (((((int)binIndex - (int)numBins()/4) % std::max<int>(1, (int)numBins()/2)) == 0)) {
//     printf("%d ", numBins());
    computeOneStage(m_stage);
    m_stage = (m_stage + 1) & 1;
  }
}

void Convolver::computeOneStage(size_t stage)
{
  if (numBins() == 1) {
    // do the whole football
    computeInput();
    for (int i=0; i < numPartitions(); ++i) {
      computeMAC(i);
    }
    computeOutput();
  } else {
    if (stage == 0) {
      // input FFT and first half of MACs
      computeInput();
      for (int i=0; i < numPartitions()/2; ++i) {
        computeMAC(i);
      }
    } else {
      // second half of MACs and output FFT
      for (int i=numPartitions()/2; i < numPartitions(); ++i) {
        computeMAC(i);
      }
      computeOutput();
    }
  }
}

void Convolver::computeInput()
{
  // get input and advance read pointer
  // NOTE: input is zero-padded automatically in pushInput
//   printf("computeInput %d %d\n", m_inputBuffer.readSpace(), m_inputBuffer.size());
  const size_t fftSize = fft()->paddedSize();
  
  // printf("computeInput rspace=%d fftSize=%d\n", m_inputBuffer.readSpace(), fftSize);
  
  // assert( m_inputBuffer.readSpace() >= fftSize );
  assert( m_inputSpecBuffer.writeSpace() >= fftSize );

  if (m_inputBuffer.readSpace() >= fftSize) {
    for (size_t c=0; c < m_inputBuffer.numChannels(); ++c)
    {
      float* src = m_inputBuffer.readVector(c);
      // perform FFT (inplace)
      fft()->execute_forward_hc(src);
      // convert from HC
      float* dst = m_inputSpecBuffer.writeVector(c);
      FFT::shufflehc(dst, src, fftSize);
      // clear MAC buffer
      memZero(m_fftMACBuffer[c], fftSize);
    }

    m_inputBuffer.readAdvance(fftSize);
  } else {
    printf("filling ...\n");
    for (size_t c=0; c < m_inputBuffer.numChannels(); ++c)
    {
      // clear spec buffer
      memZero(m_inputSpecBuffer.writeVector(c), fftSize);
      // clear MAC buffer
      memZero(m_fftMACBuffer[c], fftSize);
    }
  }  

  // save input spec pos for MAC and advance write pointer
  m_inputSpecPos = m_inputSpecBuffer.writePos();
  m_inputSpecBuffer.writeAdvance(fftSize);
}

void Convolver::computeMAC(size_t partition)
{
  // compute complex multiplication per channel and accumulate into m_fftMACBuffer
  const int fftSize       = (int)(fft()->paddedSize());
  const int partOffset    = (int)(partition * fftSize);
  const int specbufSize   = (int)(m_inputSpecBuffer.size());
  const int specbufOffset = (m_inputSpecPos - partOffset + specbufSize) % specbufSize;

  // printf("%d %d %%d\n", numBins(), partition, specbufSize, specbufOffset);

  for (size_t c = 0; c < numChannels(); ++c)
  {
    const float* specbuf = m_inputSpecBuffer.data(c) + specbufOffset;
    const float* partbuf = m_irBuffer[c] + partOffset;
    DSP::cmac_hc(m_fftMACBuffer[c], specbuf, partbuf, fftSize);
  }
}

// void Convolver::computeOutput()
// {
//   const size_t fftSize = fft()->paddedSize();
//   float* fftbuf = m_fftBuffer[0];
// 
//   for (size_t c = 0; c < numChannels(); ++c)
//   {
//     // convert to HC
//     FFT::unshufflehc(fftbuf, m_fftMACBuffer[c], fftSize);
// 
//     // perform inverse FFT of accumulated convolution results
//     fft()->execute_backward_hc(fftbuf);
// 
//     // write to output buffer with previous overlap and save current overlap
//     assert( m_outputBuffer.writeSpace() >= partitionSize() );
//     float* out = m_outputBuffer.writeVector(c);
//     float* overlap = m_overlapBuffer[c];
// 
//     // add overlap
//     // TODO: use vector ops
//     for (size_t i = 0; i < partitionSize(); ++i) {
//       out[i] = fftbuf[i] + overlap[i];
//     }
//     // save overlap
//     memCopy(overlap, fftbuf+partitionSize(), partitionSize());        
//   }
// 
//   // advance output buffer write pointer
//   m_outputBuffer.writeAdvance(partitionSize());
// }

void Convolver::computeOutput()
{
  const size_t fftSize = fft()->paddedSize();
  // float* fftbuf = m_fftBuffer[0];

  for (size_t c = 0; c < numChannels(); ++c)
  {
    // convert to HC
    FFT::unshufflehc(m_fftBuffer[c], m_fftMACBuffer[c], fftSize);
    // perform inverse FFT of accumulated convolution results
    fft()->execute_backward_hc(m_fftBuffer[c]);
  }
  
  size_t nwrite = partitionSize();
  size_t n = m_outputBuffer.writeSpace();
  
  if (n < nwrite) {
    // chunk
    for (size_t c = 0; c < numChannels(); ++c)
    {
      float* fftbuf = m_fftBuffer[c];
      float* out = m_outputBuffer.writeVector(c);
      float* overlap = m_overlapBuffer[c];
      for (size_t i = 0; i < n; ++i) {
        out[i] = fftbuf[i] + overlap[i];
      }
    }
    m_outputBuffer.writeAdvance(n);
    for (size_t c = 0; c < numChannels(); ++c)
    {
      float* fftbuf = m_fftBuffer[c];
      float* out = m_outputBuffer.writeVector(c);
      float* overlap = m_overlapBuffer[c];
      for (size_t i = n; i < nwrite; ++i) {
        out[i] = fftbuf[i] + overlap[i];
      }
      // save overlap
      memCopy(overlap, fftbuf+nwrite, nwrite);
    }    
    m_outputBuffer.writeAdvance(nwrite-n);
  } else {
    for (size_t c = 0; c < numChannels(); ++c)
    {
      float* fftbuf = m_fftBuffer[c];      
      float* out = m_outputBuffer.writeVector(c);
      float* overlap = m_overlapBuffer[c];
      for (size_t i = 0; i < nwrite; ++i) {
        out[i] = fftbuf[i] + overlap[i];
      }
      // save overlap
      memCopy(overlap, fftbuf+nwrite, nwrite);
    }
    // advance output buffer write pointer
    m_outputBuffer.writeAdvance(nwrite);
  }
}

void VEP::Convolver::process(float** dst, const float** src, size_t numChannels, size_t numFrames, size_t binPeriod)
{
  pushInput(src, numChannels, numFrames);
  compute(m_binIndex);
  pullOutput(dst, numChannels, numFrames);
  m_binIndex = (m_binIndex + 1) & binPeriod;
}

void VEP::Convolver::setKernel(const float* srcBuffer, size_t srcNumChannels, size_t srcNumFrames)
{
  const size_t minNumChannels = std::min(numChannels(), srcNumChannels);
  
  const size_t fftSize = fft()->paddedSize();
	const double norm = fft()->norm(); // normalize by 1/N

	for (size_t c = 0; c < minNumChannels; ++c)
	{
    float* dst = m_irBuffer[c];
		const float* src = srcBuffer + (irOffset() * srcNumChannels) + c;
		size_t rest = std::min(
		  srcNumFrames - std::min(srcNumFrames, irOffset()),  // src frames - offset
		  partitionSize() * numPartitions()                   // required frames
		);
    
		for (size_t pi=0; pi < numPartitions(); ++pi)
		{
			// deinterleave channel c into fftbuf and pad
      size_t n = std::min(partitionSize(), rest);
      float* fftbuf = m_fftBuffer[0];
			for (size_t i = 0; i < n; ++i)
			{
				fftbuf[i] = *src * norm;
        src += srcNumChannels;
			}
			memZero(fftbuf+n, fftSize-n);

			// transform partition
			fft()->execute_forward_hc(fftbuf);
			// convert from HC
			FFT::shufflehc(dst, fftbuf, fftSize);

			dst += fftSize;
			rest -= n;
		}
	}
	
  for (size_t c = minNumChannels; c < numChannels(); ++c)
  {
    memZero(m_irBuffer[c], fftSize * numPartitions());
  }
}

// =====================================================================
// VEP::Convolution

VEP::Convolution::Convolution(const Response& response, size_t numRTProcs)
	: m_response(response),
    m_numRTProcs(numRTProcs == 0 ? response.numModules() : numRTProcs),
		m_process(0)
{
  size_t binSize = response.minPartSize();

  size_t delay = 0;
	if (response.numModules() > m_numRTProcs) {
	  const Response::Module& module = response[m_numRTProcs];
    delay = module.offset();
  }

  // initialize convolvers
  for (size_t i=0; i < response.numModules(); ++i) {
   m_convs.push_back(
     new Convolver(
       response.numChannels(),
       binSize,
       response[i],
       i >= m_numRTProcs ? response[m_numRTProcs].offset() : 0));
  }
	
	m_binPeriod = m_convs.back()->numBins() - 1;
	assert( ISPOWEROFTWO(m_binPeriod+1) );
  m_binIndex = 0;
  m_binIndex2 = 0;
	
	if (response.numModules() > m_numRTProcs) {
	  const Response::Module& module = response[m_numRTProcs];
    size_t irOffset = module.offset();
    m_process = new Process(this, response.numChannels(), binSize, irOffset, irOffset*4);
  }
}

VEP::Convolution::~Convolution()
{
  delete m_process;
  for (ConvolverArray::iterator it = m_convs.begin(); it != m_convs.end(); ++it)
    delete *it;
}

void VEP::Convolution::process(float** dst, const float** src, size_t numChannels, size_t numFrames)
{
	//assert( (dst != src) && (dst->getSampleData(0) != src->getSampleData(0)) );

	if (m_process && !m_process->write(src, numChannels, numFrames))
	{
#ifndef NDEBUG
    printf("VEP::Convolution: couldn't write to process\n");
#endif
    size_t numTries = 100;
		while (/*(numTries-- > 0) &&*/ !m_process->write(src, numChannels, numFrames)) /* SPINLOCK */;
	}
	
  for (size_t i=0; i < std::min(m_numRTProcs, m_convs.size()); ++i)
	{
    m_convs[i]->process(dst, src, numChannels, numFrames, m_binPeriod);
  }
	
	if (m_process && !m_process->read(dst, numChannels, numFrames))
	{
#ifndef NDEBUG
      printf("VEP::Convolution: couldn't read from process\n");
#endif
    size_t numTries = 100;
		while (/*(numTries-- > 0) &&*/ !m_process->read(dst, numChannels, numFrames)) /* SPINLOCK */;
	}
}

void VEP::Convolution::process2(float** dst, const float** src, size_t numChannels, size_t numFrames)
{
  // jassert( (dst != src) && (dst->getSampleData(0) != src->getSampleData(0)) );

  for (size_t i=m_numRTProcs; i < m_convs.size(); ++i)
	{
    m_convs[i]->process(dst, src, numChannels, numFrames, m_binPeriod);
  }
}

void VEP::Convolution::setKernel(const float* data, size_t numChannels, size_t numFrames)
{
  // NOTE: NOT thread-safe!
  for (size_t i=0; i < m_convs.size(); ++i)
  {
    m_convs[i]->setKernel(data, numChannels, numFrames);
  }
}

// =====================================================================
// VEP::Convolution::Process

void set_real_time_priority(pthread_t thread);
void set_real_time_priority(pthread_t thread)
{
	int policy;
	struct sched_param param;

	pthread_getschedparam (thread, &policy, &param);
#ifdef SC_LINUX
	policy = SCHED_FIFO;
	const char* env = getenv("SC_SCHED_PRIO");
	// jack uses a priority of 10 in realtime mode, so this is a good default
	const int defprio = 5;
	const int minprio = sched_get_priority_min(policy);
	const int maxprio = sched_get_priority_max(policy);
	const int prio = env ? atoi(env) : defprio;
	param.sched_priority = sc_clip(prio, minprio, maxprio);
#else
	policy = SCHED_RR;         // round-robin, AKA real-time scheduling
	param.sched_priority = 63; // you'll have to play with this to see what it does
#endif
	pthread_setschedparam (thread, policy, &param);
}

VEP::Convolution::Process::Process(Convolution* owner, size_t numChannels, size_t binSize, size_t irOffset, size_t fifoSize)
	: m_owner(owner),
		m_numChannels(numChannels),
		m_binSize(binSize),
		m_irOffset(irOffset),
		m_initialDelay(irOffset),
		m_inFifo(numChannels, fifoSize),
		m_outFifo(numChannels, fifoSize)
{
  m_srcChannelData = new float*[m_numChannels];
  m_dstChannelData = new float*[m_numChannels];
  //m_outFifo.writeAdvance(irOffset);
  printf("Process::Process(): irOffset=%d binSize=%d rspace=%d\n", irOffset, binSize, m_outFifo.readSpace());
  pthread_create(&m_thread, 0, threadFunc, this);
  m_cond.wait();
  printf("Process: orspace=%d\n", m_outFifo.readSpace());
}

VEP::Convolution::Process::~Process()
{
  m_shouldBeRunning = false;
  m_cond.signal();
  pthread_join(m_thread, 0);
  
  delete [] m_srcChannelData;
  delete [] m_dstChannelData;
}

bool VEP::Convolution::Process::write(const float** buffer, size_t numChannels, size_t numSamples)
{
  // printf("Process::write: wspace=%d numSamples=%d\n", m_inFifo.writeSpace(), numSamples);
  
	if (m_inFifo.writeSpace() < numSamples)
		return false;

  for (size_t c=0; c < std::min(numChannels, m_numChannels); ++c) {
    memCopy(m_inFifo.writeVector(c), buffer[c], numSamples);
	}
	
  m_inFifo.writeAdvance(numSamples);
	
  m_cond.signal();
	
	return true;
}

bool VEP::Convolution::Process::read(float** buffer, size_t numChannels, size_t numSamples)
{
  // printf("Process::read: rspace=%d numSamples=%d\n", m_inFifo.readSpace(), numSamples);

  // if (m_initialDelay > 0) {
  //   assert( (m_initialDelay % numSamples) == 0 );
  //   m_initialDelay -= numSamples;
  //   return true;
  // }
  
	if (m_outFifo.readSpace() < numSamples) {
    // printf("read failure: %d < %d\n", m_outFifo.readSpace(), numSamples);
		return false;
	}
	
	for (size_t c=0; c < std::min(numChannels, m_numChannels); ++c) {
		VEP::DSP::mix(buffer[c], m_outFifo.readVector(c), numSamples);
	}
	
	m_outFifo.readAdvance(numSamples);
	
	return true;
}

void VEP::Convolution::Process::run()
{
  assert( (m_irOffset % m_binSize) == 0 );
  
  // for (size_t i=0; i < m_irOffset; i+=m_binSize)
  // {
  //  for (size_t c=0; c < m_numChannels; ++c)
  //  {
  //    m_srcChannelData[c] = m_inFifo.readVector(c);
  //     memZero(m_srcChannelData[c], m_binSize);
  //    m_dstChannelData[c] = m_outFifo.writeVector(c);
  //     memZero(m_dstChannelData[c], m_binSize);
  //  }
  //  
  //  m_owner->process2(m_dstChannelData, const_cast<const float**>(m_srcChannelData), m_numChannels, m_binSize);
  // 
  //    m_inFifo.readAdvance(m_binSize);
  //  m_outFifo.writeAdvance(m_binSize);
  // }
  
  printf("run: orspace=%d\n", m_outFifo.readSpace());
  
  set_real_time_priority(pthread_self());
  m_cond.signal();
  
  m_shouldBeRunning = true;
  while (m_shouldBeRunning)
	{
    m_cond.wait();
    
    // printf("Process::run() rspace=%d wspace=%d binSize=%D\n", m_inFifo.readSpace(), m_outFifo.writeSpace(), m_binSize);
    
		while ((m_inFifo.readSpace() >= m_binSize) && (m_outFifo.writeSpace() >= m_binSize))
		{
      if (!m_shouldBeRunning) return;
      
      // printf("Process::run() processing\n");
			for (size_t c=0; c < m_numChannels; ++c)
			{
				m_srcChannelData[c] = m_inFifo.readVector(c);
				m_dstChannelData[c] = m_outFifo.writeVector(c);
        memZero(m_dstChannelData[c], m_binSize);
			}
			
			m_owner->process2(m_dstChannelData, const_cast<const float**>(m_srcChannelData), m_numChannels, m_binSize);

			m_inFifo.readAdvance(m_binSize);
			m_outFifo.writeAdvance(m_binSize);
		}
	}
}

void* VEP::Convolution::Process::threadFunc(void* self)
{
  assert(self != 0);
  ((Convolution::Process*)self)->run();
  return 0;
}

// EOF
