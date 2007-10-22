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

#include "VEPFFT.h"
#include "VEP.h"

#include <vector>

#define VEP_FFT_DEBUG 0

using namespace VEP;

FFT::FFT(size_t logSize, bool measure)
  : m_logSize(logSize),
    m_size(1<<logSize),
    m_paddedSize(m_size<<1),
    m_norm(1./double(m_paddedSize))
{
  float* buffer = memAlloc<float>(paddedSize());
  int fftwFlags = measure ? FFTW_MEASURE : FFTW_ESTIMATE;
  m_planF = fftwf_plan_r2r_1d(paddedSize(), buffer, buffer, FFTW_R2HC, fftwFlags);
  assert( m_planF != 0 );
  m_planB = fftwf_plan_r2r_1d(paddedSize(), buffer, buffer, FFTW_HC2R, fftwFlags);
  assert( m_planB != 0 );
  memFree<float>(buffer);

#if VEP_FFT_DEBUG
  printf("FFT: f%d\n", m_paddedSize);
  fftwf_print_plan(m_planF);
  printf("\n");
  printf("FFT: b%d\n", m_paddedSize);
  fftwf_print_plan(m_planB);
  printf("\n");
#endif // VEP_FFT_DEBUG
}

FFT* FFT::get(size_t logSize, bool measure)
{
  static FFT* gFFT[kMaxLogSize+1];
  if (logSize > kMaxLogSize)
    return 0;
  if (gFFT[logSize] == 0)
    gFFT[logSize] = new FFT(logSize, measure);
  return gFFT[logSize];
}

// EOF
