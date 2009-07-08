/*  -*- mode: c++; indent-tabs-mode: nil; c-basic-offset: 4 -*-
    vim: et sta sw=4:

    skUG - SuperCollider UGen Library
    Copyright (c) 2005-2008 Stefan Kersten. All rights reserved.

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

#include <SC_PlugIn.h>
#include <math.h>

static InterfaceTable *ft;

struct SinINLF : public Unit
{
    float m_prev;
};

extern "C"
{
    void load(InterfaceTable *inTable);
    void SinINLF_Ctor(SinINLF *unit);
    void SinINLF_next(SinINLF *unit, int inNumSamples);
};

void SinINLF_Ctor(SinINLF* unit)
{
    unit->m_prev = 0;
    SETCALC(SinINLF_next);
}

void SinINLF_next(SinINLF* unit, int inNumSamples)
{
    const int n = unit->mNumInputs - 1;
    float* in   = IN(0);
    float prev = unit->m_prev;
    float fb = IN0(1);
    float* out  = OUT(0);
    while (inNumSamples--) {
        prev = *in++ + fb * prev;
        for (int i=0; i < n; ++i) {
            prev = sinf(ZIN0(2+i)*prev);
        }
        *out++ = prev;
    }
    unit->m_prev = prev;
}

void load(InterfaceTable *inTable)
{
    ft = inTable;
    DefineSimpleUnit(SinINLF);
}
