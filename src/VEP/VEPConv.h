// -*- c++ -*-
//
// VEP binaural rendering engine
//
// Copyright (C) 2005-2006 Stefan Kersten <sk@k-hornz.de>
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

#ifndef VEP_CONV_H_INCLUDED
#define VEP_CONV_H_INCLUDED

#include "VEP.h"
#include "VEPBuffer.h"
#include "VEPFFT.h"
#include "VEPRingBuffer.h"

#include "SC_PlugIn.h"
#include "SC_SyncCondition.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include <vector>

using namespace VEP;

// =====================================================================
// VEP::Response
//
// Encapsulates impulse response partitioning scheme.
namespace VEP
{
  class Response
  {
  public:
    struct Module
    {
      size_t    offset;
      size_t    size;
      size_t    count;
      FFT*      fft;
    };
    typedef std::vector<Module> ModuleArray;
  
  public:
    Response(size_t numChannels, size_t numFrames, size_t minPartSize, size_t maxPartSize);
    ~Response();
  
    // number of channels
    size_t numChannels() const { return m_numChannels; }
    size_t numFrames() const { return m_numFrames; }
    size_t minPartSize() const { return m_minPartSize; }
    size_t maxPartSize() const { return m_maxPartSize; }
  
    // size of partitioned IR
    size_t size() const { return m_size; }
    // complex frames necessary to represent partitioned IR
    size_t paddedSize() const { return size() << 1; }

    // number of different partition sizes (modules)
    size_t numModules() const { return m_modules.size(); }
    // modules
    const ModuleArray& modules() const { return m_modules; }
    const Module& module(size_t i) const { return m_modules[i]; }
  
    // total number of partitions
    size_t numPartitions() const { return m_numPartitions; }

    const AudioBuffer& data() const { return *m_data; }
    
    void transformBuffer(const float* src, size_t srcNumChannels, size_t srcNumFrames);
    void printOn(FILE *stream);

  protected:
    void initModules(size_t numFrames, size_t minSize, size_t maxSize);
    size_t addModule(size_t offset, size_t size, size_t maxCount, size_t rest);

  private:
    size_t        m_numChannels;
    size_t        m_numFrames;
    size_t        m_minPartSize;
    size_t        m_maxPartSize;
    size_t        m_size;
    size_t        m_numPartitions;
    ModuleArray   m_modules;
    AudioBuffer*  m_data;
    float*        m_fftbuf;
  };
  
  // =====================================================================
  // Convolver
  //
  // Convolution process for specific partition size.

  class Convolver
  {
  public:
    Convolver(size_t numChannels,
                  size_t binSize,             // smallest partition size N0
                  size_t numBins,             // number of bins in this partition size
                  size_t numPartitions,       // number of partitions in this convolver
                  size_t irOffset,           // offset in frames from beginning of IR
                  const VEP::FFT* fft
                  );
    void release(InterfaceTable *ft, World *world);

    size_t numChannels() const { return m_numChannels; }
    size_t binSize() const { return m_binSize; }
    size_t numBins() const { return m_numBins; }
    size_t numPartitions() const { return m_numPartitions; }
    size_t lastPartition() const { return m_numPartitions - 1; }
    size_t partitionSize() const { return m_partitionSize; }
    size_t irOffset() const { return m_irOffset; }

    // write time-domain input data
    void pushInput(const float** src, size_t numChannels, size_t numFrames);

    // read time-domain output data
    void pullOutput(float** dst, size_t numChannels, size_t numFrames);

    // do one partial convolution
    void compute(const AudioBuffer& ir, size_t binIndex);

    // switch IRs
    void setKernel(const float* data, size_t numChannels, size_t numFrames);
    
  protected:
    void computeOneStage(const AudioBuffer& ir, size_t stage);
    void computeInput();
    void computeMAC(const AudioBuffer& ir, size_t partition);
    void computeOutput();

  private:
    const VEP::FFT*         m_fft;
    size_t                  m_numChannels;
    size_t                  m_binSize;
    size_t                  m_numBins;
    size_t                  m_numPartitions;
    size_t                  m_partitionSize;
    size_t                  m_irOffset;
    AudioRingBuffer         m_inputBuffer;
    AudioRingBuffer         m_inputSpecBuffer;
    AudioRingBuffer         m_outputBuffer;
    AudioBuffer             m_irBuffer;
    AudioBuffer             m_fftMACBuffer;
    AudioBuffer             m_overlapBuffer;
    AudioBuffer             m_fftBuffer;
    size_t                  m_inputSpecPos;
    size_t                  m_stage;
  };

  // =====================================================================
  // VEP::Convolution
  //
  // Convolution process.
  
  class Convolution
  {
  public:
  	class Process
  	{
  	public:
  		Process(Convolution* owner, size_t numChannels, size_t binSize, size_t irOffset, size_t fifoSize);
      ~Process();
      
  		bool write(const float** buffer, size_t numChannels, size_t numSamples);
  		bool read(float** buffer, size_t numChannels, size_t numSamples);

		private:
      static void* threadFunc(void*);
  		void run();
      
  	private:
  		Convolution*  					m_owner;
  		size_t									m_numChannels;
  		size_t									m_binSize;
      size_t                  m_irOffset;
  		// thread state
      pthread_t               m_thread;
      Condition               m_cond;
      bool                    m_shouldBeRunning;
      // buffers
  		RingBuffer<float,true>  m_inFifo;
  		RingBuffer<float,true>  m_outFifo;
      float**                 m_srcChannelData;
      float**                 m_dstChannelData;
  	};
	
  	typedef std::vector<Convolver*> ConvolverArray;
	
  public:
  	Convolution(Response* response, size_t numRTProcs=1);
  	~Convolution();
	
  	// PRE: dst != src
  	void process(float** dst, const float** src, size_t numChannels, size_t numFrames);
    void setKernel(const float* data, size_t numChannels, size_t numFrames);
    
  protected:
  	friend class Process;
  	void process2(float** dst, const float** src, size_t numChannels, size_t numFrames);

  private:
  	ConvolverArray			m_convs;
  	size_t							m_numRTProcs;
  	size_t							m_binPeriod;
  	size_t							m_binPeriod2;
  	size_t							m_binIndex;
  	size_t							m_binIndex2;
  	Process*						m_process;
    size_t              m_processDelayBins;
    Response*           m_response;
  };
};

#endif // VEP_CONV_H_INCLUDED
