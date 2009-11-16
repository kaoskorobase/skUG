/*  -*- mode: c++; indent-tabs-mode: nil; c-basic-offset: 4 -*-
    vim: et sta sw=4:

    skUG - SuperCollider UGen Library
    Copyright (c) 2005-2009 Stefan Kersten. All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
    USA
*/

#include "SKUG.h"
#include <sc_msg_iter.h>
#include <math.h>
#include <speex/speex_preprocess.h>
#include <inttypes.h>

using namespace SKUG;
static InterfaceTable *ft;

struct VAD : public Unit
{
    SpeexPreprocessState* m_speex;
    int16_t*              m_buffer;
};

extern "C"
{
    void load(InterfaceTable *inTable);
    void VAD_Ctor(VAD *unit);
    void VAD_next(VAD *unit, int inNumSamples);
};

void VAD_Ctor(VAD* unit)
{
    unit->m_speex = speex_preprocess_state_init(BUFLENGTH, SAMPLERATE);
    speex_preprocess_ctl(unit->m_speex, SPEEX_PREPROCESS_SET_VAD, 0);
    unit->m_buffer = (int16_t*)RTAlloc(unit->mWorld, BUFLENGTH * sizeof(int16_t));
    SETCALC(VAD_next);
}

void VAD_Dtor(VAD* unit)
{
    speex_preprocess_state_destroy(unit->m_speex);
    RTFree(unit->mWorld, unit->m_buffer);
}

inline void convert_f2s(int16_t* dst, float* src, size_t n)
{
    const float r = (float)0x7FFF;
    while (n--) *dst++ = (int16_t)(*src++ * r);
}

inline void convert_s2f(float* dst, int16_t* src, size_t n)
{
    const float r = (float)(1.0/(double)0x7FFF);
    while (n--) *dst++ = (float)*src++ * r;
}

void VAD_next(VAD* unit, int inNumSamples)
{
    float* in = IN(0);
    float* out = OUT(0);
    float* vad = OUT(1);
    
    convert_f2s(unit->m_buffer, in, inNumSamples);
    float act = (float)speex_preprocess_run(unit->m_speex, unit->m_buffer);
    convert_s2f(out, unit->m_buffer, inNumSamples);
    while (inNumSamples--) *vad++ = act;
}

inline int normalize_rot(int rot, int n)
{
    while (rot > n) rot -= n;
    while (rot < -n) rot += n;
    return rot;
}

void b_gen_rotate(World* world, SndBuf* buf, sc_msg_iter* msg)
{
    const int frames   = buf->frames;
    const int channels = buf->channels;
    const int rot      = normalize_rot((int)msg->getf(0), frames);
    const int arot     = std::abs(rot);
    const int remain   = frames - arot;
    
    // printf("rotate: %f %d\n", rot0, rot);
    
    if (arot > frames/2) {
        float* tmp = rtAlloc<float>(ft, world, remain*channels);
        if (rot > 0) {
            memCopy(tmp                         , buf->data                   , remain*channels);
            memMove(buf->data                   , buf->data + remain*channels , arot*channels  );
            memCopy(buf->data + arot*channels   , tmp                         , remain*channels);
        } else if (rot < 0) {
            memCopy(tmp                         , buf->data + arot * channels , remain*channels);
            memMove(buf->data + remain*channels , buf->data                   , arot*channels  );
            memCopy(buf->data                   , tmp                         , remain*channels);
        }
        rtFree(ft, world, tmp);
    } else if (rot < 0) {
        float* tmp = rtAlloc<float>(ft, world, arot*channels);
        if (rot > 0) {
            memCopy(tmp                         , buf->data + remain*channels , arot*channels  );
            memMove(buf->data + arot*channels   , buf->data                   , remain*channels);
            memCopy(buf->data                   , tmp                         , arot*channels  );
        } else if (rot < 0) {
            memCopy(tmp                         , buf->data                 , arot*channels  );
            memMove(buf->data                   , buf->data + arot*channels , remain*channels);
            memCopy(buf->data + remain*channels , tmp                       , arot*channels  );
        }
        rtFree(ft, world, tmp);
    }
    // if (rot > 0) {
    //     if (arot > frames/2) {
    //         float* tmp = rtAlloc<float>(ft, world, remain*channels);
    //         memCopy(tmp, buf->data, remain*channels);
    //         memMove(buf->data, buf->data + remain*channels, arot*channels);
    //         memCopy(buf->data + arot*channels, tmp, remain*channels);
    //         rtFree(ft, world, tmp);
    //     } else {
    //         float* tmp = rtAlloc<float>(ft, world, arot*channels);
    //         memCopy(tmp                       , buf->data + remain*channels , arot*channels  );
    //         memMove(buf->data + arot*channels , buf->data                   , remain*channels);
    //         memCopy(buf->data                 , tmp                         , arot*channels  );
    //         rtFree(ft, world, tmp);
    //     }
    // } else if (rot < 0) {
    //     if (arot > frames/2) {
    //         float* tmp = rtAlloc<float>(ft, world, remain*channels);
    //         memCopy(tmp, buf->data + arot * channels, remain*channels);
    //         memMove(buf->data + remain*channels, buf->data, arot*channels);
    //         memCopy(buf->data, tmp, remain*channels);
    //         rtFree(ft, world, tmp);
    //     } else {
    //         float* tmp = rtAlloc<float>(ft, world, arot*channels);
    //         memCopy(tmp, buf->data, arot*channels);
    //         memMove(buf->data, buf->data + arot*channels, remain*channels);
    //         memCopy(buf->data + remain*channels, tmp, arot*channels);
    //         rtFree(ft, world, tmp);
    //     }
    // }
}

void load(InterfaceTable *inTable)
{
    ft = inTable;
    DefineDtorUnit(VAD);
    DefineBufGen("rotate", &b_gen_rotate);
}
