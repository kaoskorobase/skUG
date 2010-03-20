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

using namespace SKUG;
static InterfaceTable *ft;

extern "C"
{
    void load(InterfaceTable *inTable);
};

inline int normalize_rot(int rot, int n)
{
    while (rot > n) rot -= n;
    while (rot < -n) rot += n;
    return rot;
}

/** Rotate the samples in a buffer by 'n'.
    
    Negative shifts rotate to the left. */
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
}

void load(InterfaceTable *inTable)
{
    ft = inTable;
    DefineBufGen("rotate", &b_gen_rotate);
}
