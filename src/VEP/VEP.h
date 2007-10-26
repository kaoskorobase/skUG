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

#ifndef VEP_H_INCLUDED
#define VEP_H_INCLUDED

#include "SC_PlugIn.h"

#include <stdexcept>
#include <sys/time.h>

namespace VEP
{
  // ===================================================================
  // Typesafe memory allocation

  template <class T> inline T* rtAlloc(InterfaceTable *ft, World *world, size_t n)
  {
//     printf("rtAlloc %d\n", n * sizeof(T));
    T *ptr = (T*)RTAlloc(world, n * sizeof(T));
    //T *ptr = (T*)fftwf_malloc(n * sizeof(T));
    if (ptr == 0) throw std::runtime_error("rtAlloc failed");
    return ptr;
  }
  template <class T> inline void rtFree(InterfaceTable *ft, World *world, T *ptr)
  {
    RTFree(world, ptr);
    //fftwf_free(ptr);
  }

  // ===================================================================
  // Typesafe memory initialization
  
  template <class T> inline void memCopy(T *dst, const T *src, size_t n)
  {
    memcpy(dst, src, n * sizeof(T));
  }
//   template <> inline void memCopy<float>(float *dst, const float *src, size_t n)
//   {
//     Copy(n, dst, const_cast<float*>(src), n);
//   }
  template <class T> inline void memZero(T *ptr, size_t n=1)
  {
    memset(ptr, 0, n * sizeof(T));
  }
//   template <> inline void memZero<float>(float *ptr, size_t n)
//   {
//     Clear(n, ptr);
//   }

  // ===================================================================
  // Printing

  void print(float *src, size_t size, const char *tag="float");

  // =====================================================================
  // VEP::Condition
  //
  // Thread signalling.
  
  class Condition
  {
  public:
    Condition()
    {
      pthread_mutex_init(&m_mutex, 0);
      pthread_cond_init(&m_cond, 0);
    }
    ~Condition()
    {
      pthread_mutex_destroy(&m_mutex);
      pthread_cond_destroy(&m_cond);
    }
    
    void wait()
    {
      pthread_mutex_lock(&m_mutex);
      pthread_cond_wait(&m_cond, &m_mutex);
      pthread_mutex_unlock(&m_mutex);
    }
    void signal()
    {
      pthread_cond_signal(&m_cond);
    }
    
  private:
    pthread_mutex_t m_mutex;
    pthread_cond_t  m_cond;
  };
  
  // ===================================================================
  // Timer
  //
  // Simple system timer interface.

  class Timer
  {
  public:
    static double time()
    {
      struct timeval tv;
      gettimeofday(&tv, 0);
      return (double)tv.tv_sec + (double)tv.tv_usec * 1e-6;
    }

  public:
    Timer() { reset(); }

    void reset() { m_start = time(); }
    double start() const { m_start; }
    double delta() const { return time() - m_start; }

  private:
    double m_start;
  };

  // ===================================================================
  // Benchmark
  //
  // Encapsulates a number of time probes.

  class Benchmark
  {
  public:
    Benchmark(size_t size)
      : m_size(size),
        m_time(0.),
        m_count(0)
    {
    }

    bool atEnd() const
    {
      return m_count == m_size;
    }

    void begin()
    {
      m_timer.reset();
    }
    void end()
    {
      m_time += m_timer.delta();
      m_count++;
    }

    double avg() const
    {
      return m_count ? m_time/(double)m_count : 0.;
    }

    void printSummary(FILE *stream, const char *tag)
    {
      fprintf(stream, "BENCH %s: %.6f\n", tag, avg());
    }

  private:
    size_t	m_size;
    double	m_time;
    uint64_t	m_count;
    Timer	m_timer;
  };

  // ===================================================================
  // PeriodicBenchmark
  //
  // Encapsulates a number of time probes with periodic boundaries.

  class PeriodicBenchmark
  {
  public:
    void init(size_t period)
    {
      m_period = period;
      m_time = 0.;
      m_count = 0;
    }

    bool atBoundary() const
    {
      return (m_count % m_period) == 0;
    }

    void begin()
    {
      m_timer.reset();
    }
    void end()
    {
      m_time += m_timer.delta();
    }
    void inc()
    {
      m_count++;
    }

    double avg() const
    {
      return m_count ? m_time/(double)m_count : 0.;
    }

    void printSummary(FILE *stream, const char *tag)
    {
      if (atBoundary()) {
        fprintf(stream, "BENCH %s %.6f\n", tag, avg());
      }
    }

  private:
    size_t	m_period;
    double	m_time;
    uint64_t	m_count;
    Timer	m_timer;
  };
};

#endif // VEP_H_INCLUDED
