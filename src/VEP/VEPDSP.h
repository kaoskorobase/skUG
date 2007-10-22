// -*- c++ -*-
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

#ifndef VEP_DSP_HH_INCLUDED
#define VEP_DSP_HH_INCLUDED

namespace VEP
{
  namespace DSP
  {
    inline static void mix_f(float* dst, const float* src, size_t n)
    {
      while (n--) *dst++ += *src++;
    }
  
    inline static void cmac_hc_f(float *dst, const float *src1, const float *src2, size_t n)
    {
      float d0 = dst[0] + src1[0] * src2[0];
      float d4 = dst[4] + src1[4] * src2[4];

      for (size_t i = 0; i < n; i += 8) {
        dst[i+0] += src1[i+0] * src2[i+0] - src1[i+4] * src2[i+4];
        dst[i+1] += src1[i+1] * src2[i+1] - src1[i+5] * src2[i+5];
        dst[i+2] += src1[i+2] * src2[i+2] - src1[i+6] * src2[i+6];
        dst[i+3] += src1[i+3] * src2[i+3] - src1[i+7] * src2[i+7];

        dst[i+4] += src1[i+0] * src2[i+4] + src1[i+4] * src2[i+0];
        dst[i+5] += src1[i+1] * src2[i+5] + src1[i+5] * src2[i+1];
        dst[i+6] += src1[i+2] * src2[i+6] + src1[i+6] * src2[i+2];
        dst[i+7] += src1[i+3] * src2[i+7] + src1[i+7] * src2[i+3];
      }

      dst[0] = d0;
      dst[4] = d4;
    }
  
#if defined(__ALTIVEC__)

# include "SC_Altivec.h"

    inline static void mix(float* dst, const float* src, size_t n)
    {
      vfloat32* vdst = (vfloat32*)dst;
      vfloat32* vsrc = (vfloat32*)src;
      
      if (n < 16) {
    		for (size_t i=0; i < n>>2; ++i) {
          vfloat32 vdst1 = *vdst;
          vfloat32 vsrc1 = *vsrc;
    			vdst1 = vec_add(vdst1, vsrc1);
          *vdst = vdst1;
          ++vdst;
          ++vsrc;
    		}
    	} else {
    		for (size_t i=0; i < n>>4; ++i) {
    			vfloat32 vdst1 = vdst[0];
    			vfloat32 vsrc1 = vsrc[0];
    			vfloat32 vdst2 = vdst[1];
    			vfloat32 vsrc2 = vsrc[1];
    			vfloat32 vdst3 = vdst[2];
    			vfloat32 vsrc3 = vsrc[2];
    			vfloat32 vdst4 = vdst[3];
    			vfloat32 vsrc4 = vsrc[3];
    			vdst1 = vec_add(vdst1, vsrc1);
    			vdst2 = vec_add(vdst2, vsrc2);
    			vdst3 = vec_add(vdst3, vsrc3);
    			vdst4 = vec_add(vdst4, vsrc4);
          vdst[0] = vdst1;
          vdst[1] = vdst2;
          vdst[2] = vdst3;
          vdst[3] = vdst4;
    			vdst += 4;
    			vsrc += 4;
    		}
  	  }
    }
  
    inline static void cmac_hc(float *dst, const float *src1, const float *src2, size_t n)
    {
      vfloat32 *vsrc1 = (vfloat32*)src1;
      vfloat32 *vsrc2 = (vfloat32*)src2;
      vfloat32 *vdst  = (vfloat32*)dst;
      define_vzero;
      define_vones;

      float d0 = dst[0] + src1[0] * src2[0];
      float d4 = dst[4] + src1[4] * src2[4];

      for (size_t i=0; i < n>>2; i += 2) {
        vfloat32 c0_a = *(vsrc1+i+0);
        vfloat32 c0_b = *(vsrc1+i+1);
        vfloat32 c1_a = *(vsrc2+i+0);
        vfloat32 c1_b = *(vsrc2+i+1);
        vfloat32 cd_a = *( vdst+i+0);
        vfloat32 cd_b = *( vdst+i+1);
        *(vdst+i+0) = vec_madd(vec_sub(vec_mul(c0_a, c1_a), vec_mul(c0_b, c1_b)), vones, cd_a);
        *(vdst+i+1) = vec_madd(vec_add(vec_mul(c0_b, c1_a), vec_mul(c0_a, c1_b)), vones, cd_b);
      }

      dst[0] = d0;
      dst[4] = d4;
    }

#elif defined(__SSE__)

# include <xmmintrin.h>

    typedef __m128 vfloat32;

    inline static void mix(float* dst, const float* src, size_t n)
    {
  		for (size_t i=0; i < n; i+=4) {
  			vfloat32 vsrc = _mm_load_ps(src);
  			vfloat32 vdst = _mm_load_ps(dst);
  			vdst = _mm_add_ps(vdst, vsrc);
  			_mm_store_ps(dst, vdst);
  			dst += 4; src += 4;
  		}
    }
  
    inline static void cmac_hc(float *dst, const float *src1, const float *src2, size_t n)
    {
      vfloat32 *vsrc1 = (vfloat32*)src1;
      vfloat32 *vsrc2 = (vfloat32*)src2;
      vfloat32 *vdst  = (vfloat32*)dst;

      float d0 = dst[0] + src1[0] * src2[0];
      float d4 = dst[4] + src1[4] * src2[4];

      for (int i=0; i < n>>2; i += 2) {
        vfloat32 c0_a = *(vsrc1+i+0);
        vfloat32 c0_b = *(vsrc1+i+1);
        vfloat32 c1_a = *(vsrc2+i+0);
        vfloat32 c1_b = *(vsrc2+i+1);
        vfloat32 cd_a = *( vdst+i+0);
        vfloat32 cd_b = *( vdst+i+1);
        *(vdst+i+0) = _mm_add_ps(_mm_sub_ps(_mm_mul_ps(c0_a, c1_a), _mm_mul_ps(c0_b, c1_b)), cd_a);
        *(vdst+i+1) = _mm_add_ps(_mm_add_ps(_mm_mul_ps(c0_b, c1_a), _mm_mul_ps(c0_a, c1_b)), cd_b);
      }

      dst[0] = d0;
      dst[4] = d4;
    }

#else // !(ALTIVEC || SSE)
    inline static void mix(float* dst, const float* src, size_t n)
    {
      mix_f(dst, src, n);
    }
  
    inline static void cmac_hc(float *dst, const float *src1, const float *src2, size_t n)
    {
      cmac_hc_f(dst, src1, src2, n);
    }
#endif

  }; // namespace DSP
}; // namespace VEP

#endif // VEP_DSP_HH_INCLUDED
