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

#ifndef VEP_FFT_H_INCLUDED
#define VEP_FFT_H_INCLUDED

#include <fftw3.h>

namespace VEP
{
  class FFT
  {
  public:
    enum {
      kMaxLogSize = 16 // mucho
    };
    
  public:
    // shuffle data from HC format in src to dst in SIMD format
    // PRE: ISPOWEROFTWO(N) and N >= 8
    static inline void shufflehc(float *dst, const float *src, size_t N);
    // unshuffle data from SIMD format in src to dst in HC format
    // PRE: ISPOWEROFTWO(N) and N >= 8
    static inline void unshufflehc(float *dst, const float *src, size_t N);

    static FFT* get(size_t logSize, bool measure);
    
  public:
    size_t logSize() const { return m_logSize; }
    size_t size() const { return m_size; }
    size_t paddedSize() const { return m_paddedSize; }
    double norm() const { return m_norm; }

    fftwf_plan planForward() { return m_planF; }
    fftwf_plan planBackward() { return m_planB; }

    inline void execute_forward_hc(float *io) const;
    inline void execute_backward_hc(float *io) const;

  private:
    FFT(size_t logSize, bool measure);
    ~FFT();

  private:
    size_t                m_logSize;      // FFT log size
    size_t                m_size;         // FFT size (2^logSize)
    size_t                m_paddedSize;   // FFT size * 2
    fftwf_plan            m_planF;        // forward plan (real -> complex)
    fftwf_plan            m_planB;        // backward plan (complex -> real)
    double                m_norm;         // normalization factor (1/sqrt(N))
  };

  template <class T> static T* memAlloc(size_t n)
  {
    // TODO: check for errors
    return (T*)fftwf_malloc(n*sizeof(T));
  }
  template <class T> static void memFree(T* ptr)
  {
    fftwf_free(ptr);
  }  
};

inline void VEP::FFT::shufflehc(float *dst, const float *src, size_t N)
{
  const size_t N2 = N>>1;

  // re
  dst[0] = src[0];      // dc
  dst[1] = src[1];
  dst[2] = src[2];
  dst[3] = src[3];

  // im
  dst[4] = src[N2];     // nyq
  dst[5] = src[N-1];
  dst[6] = src[N-2];
  dst[7] = src[N-3];

  for (size_t di=8, si=4; si < N2; di+=8, si+=4) {
    // re
    dst[di+0] = src[si+0];
    dst[di+1] = src[si+1];
    dst[di+2] = src[si+2];
    dst[di+3] = src[si+3];
    // im
    dst[di+4] = src[N-si-0];
    dst[di+5] = src[N-si-1];
    dst[di+6] = src[N-si-2];
    dst[di+7] = src[N-si-3];
  }
}

inline void VEP::FFT::unshufflehc(float *dst, const float *src, size_t N)
{
  const size_t N2 = N>>1;

  // re
  dst[0] = src[0];      // dc
  dst[1] = src[1];
  dst[2] = src[2];
  dst[3] = src[3];

  // im
  dst[N2]  = src[4];    // nyq
  dst[N-1] = src[5];
  dst[N-2] = src[6];
  dst[N-3] = src[7];

  for (size_t di=4, si=8; di < N2; di+=4, si+=8) {
    // re
    dst[di+0] = src[si+0];
    dst[di+1] = src[si+1];
    dst[di+2] = src[si+2];
    dst[di+3] = src[si+3];
    // im
    dst[N-di-0] = src[si+4];
    dst[N-di-1] = src[si+5];
    dst[N-di-2] = src[si+6];
    dst[N-di-3] = src[si+7];
  }
}

inline void VEP::FFT::execute_forward_hc(float *io) const
{
  fftwf_execute_r2r(m_planF, io, io);
}

inline void VEP::FFT::execute_backward_hc(float *io) const
{
  fftwf_execute_r2r(m_planB, io, io);
}

#endif // VEP_FFT_H_INCLUDED
