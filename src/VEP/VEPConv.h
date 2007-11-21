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
    class Module
    {
    public:
      Module(size_t offset, size_t size, size_t count, const FFT* fft)
        : m_offset(offset),
          m_size(size),
          m_count(count),
          m_fft(fft)
      { }
      
      size_t offset() const { return m_offset; }
      size_t size() const { return m_size; }
      size_t count() const { return m_count; }
      const FFT* fft() const { return m_fft; }
      
    private:
      size_t      m_offset;
      size_t      m_size;
      size_t      m_count;
      const FFT*  m_fft;
    };
    
    typedef std::vector<Module> ModuleArray;
  
  public:
    Response(size_t numChannels, size_t numFrames, size_t minPartSize, size_t maxPartSize);
  
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
  };
  
  // =====================================================================
  // Convolver
  //
  // Convolution process for specific partition size.

  class Convolver
  {
  public:
    Convolver(size_t numChannels,
              // smallest partition size N0
              size_t binSize,
              const Response::Module& module);
    void release(InterfaceTable *ft, World *world);

    size_t numChannels() const { return m_numChannels; }
    size_t binSize() const { return m_binSize; }
    size_t numBins() const { return partitionSize()/binSize(); }
    size_t numPartitions() const { return m_module.count(); }
    size_t lastPartition() const { return numPartitions() - 1; }
    size_t partitionSize() const { return m_module.size(); }
    size_t irOffset() const { return m_module.offset(); }
    
    const FFT* fft() const { return m_module.fft(); }
    size_t fftSize() const { return fft()->paddedSize(); }
    
    // write time-domain input data
    void pushInput(const float** src, size_t numChannels, size_t numFrames);

    // read time-domain output data
    void pullOutput(float** dst, size_t numChannels, size_t numFrames);

    // do one partial convolution
    void compute(size_t binIndex);

    // switch IRs
    void setKernel(const float* data, size_t numChannels, size_t numFrames);
    
  protected:
    void computeOneStage(size_t stage);
    void computeInput();
    void computeMAC(size_t partition);
    void computeOutput();

  private:
    size_t                  m_numChannels;
    size_t                  m_binSize;
    Response::Module        m_module;
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
