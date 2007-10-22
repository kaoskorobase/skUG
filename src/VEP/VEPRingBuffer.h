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

#ifndef VEP_RINGBUFFER_H_INCLUDED
#define VEP_RINGBUFFER_H_INCLUDED

#include "VEPFFT.h"
#include <vector>

#ifdef SC_DARWIN
# include <CoreServices/CoreServices.h>
#endif

namespace VEP
{
  template <class T, bool useBarrier=false> class RingBuffer
  {
  public:
    typedef std::vector<T*> Data;
    typedef typename Data::iterator iterator;
    typedef typename Data::const_iterator const_iterator;
    
  public:
    RingBuffer()
    {
      init(0, 0);
    }

    RingBuffer(size_t numChannels, size_t numFrames, bool clear=true)
    {
      init(numChannels, numFrames, clear);
    }
    
    ~RingBuffer()
    {
      for (iterator it = m_data.begin(); it != m_data.end(); ++it)
      {
        memFree<T>(*it);
      }
    }
    
    size_t numChannels() { return m_data.size(); }
    
    // return total capacity
    size_t size() const { return m_size; }

    // return pointer to data
    T *data(size_t i) { return m_data[i]; }

    // return current read position
    size_t readPos() const { return m_readPos; }

    // return /continuous/ read space
    size_t readSpace() const
    {
      return m_readPos <= m_writePos ? m_writePos - m_readPos : m_size - m_readPos;
    }

    // return read vector for continuous reading
    T *readVector(size_t i)
    {
      return m_data[i] + m_readPos;
    }

    // advance read pointer by n
    void readAdvance(size_t n)
    {
      size_t nextPos = (m_readPos + n) % m_size;
      if (useBarrier) {
#if defined SC_DARWIN
  		  CompareAndSwap(m_readPos, nextPos, &m_readPos);
#elif defined SC_WIN32
  		  InterlockedExchange(reinterpret_cast<volatile LONG*>(&m_readPos), nextPos);
#else
  		  m_readPos = nextPos;
#endif
      } else {
  		  m_readPos = nextPos;
      }
    }

  	size_t read(T** dst, size_t inNumChannels, size_t n)
  	{
  		size_t remain = n;
  		size_t rs;
  		while ((remain > 0) && ((rs = readSpace()) > 0))
  		{
  			size_t rn = std::min(rs, remain);
        for (size_t c = 0; c < std::min(inNumChannels, numChannels()); ++c)
        {
  			  memCopy(dst[c], readVector(c), rn);
    			dst[c] += rn;
  			}
  			readAdvance(rn);
  			remain -= rn;
  		}
  		return n - remain;
  	}

    // return current write position
    size_t writePos() const { return m_writePos; }

    // return /continuous/ write space
    size_t writeSpace() const
    {
      return m_writePos < m_readPos ? m_readPos - m_writePos - 1 : m_size - m_writePos;
    }

    // return write vector for continuous writing
    T *writeVector(size_t i)
    {
      return m_data[i] + m_writePos;
    }

    // advance write pointer by n
    void writeAdvance(size_t n)
    {
      size_t nextPos = (m_writePos + n) % m_size;
      if (useBarrier) {
#if defined SC_DARWIN
  		  CompareAndSwap(m_writePos, nextPos, &m_writePos);
#elif defined SC_WIN32
  		  InterlockedExchange(reinterpret_cast<volatile LONG*>(&m_writePos), nextPos);
#else
  		  m_writePos = nextPos;
#endif
      } else {
  		  m_writePos = nextPos;
      }
    }

  	size_t write(const T* src, size_t inNumChannels, size_t n)
  	{
  		size_t remain = n;
  		size_t ws;
  		while ((remain > 0) && ((ws = writeSpace()) > 0))
  		{
  			size_t wn = std::min(ws, remain);
        for (size_t c=0; c < std::min(inNumChannels, numChannels()); ++c)
        {
  			  memCopy(writeVector(c), src[c], wn);
    			src[c] += wn;
  			}
  			writeAdvance(wn);
  			remain -= wn;
  		}
  		return n - remain;
  	}
  	
  protected:
    // initalize ringbuffer
    void init(size_t numChannels, size_t numFrames, bool clear=true)
    {
      m_data.reserve(numChannels);
      for (size_t i=0; i < numChannels; ++i)
      {
        m_data.push_back(memAlloc<T>(numFrames));
        if (clear) memset(m_data.back(), 0, numFrames*sizeof(T));
      }
      m_size = numFrames;
      m_readPos = 0;
      m_writePos = 0;
    }
    
  private:
    Data          m_data;
    size_t        m_size;
    size_t        m_readPos;
    size_t        m_writePos;
  };
  
  typedef RingBuffer<float> AudioRingBuffer;
};

#endif // VEP_RINGBUFFER_H_INCLUDED
