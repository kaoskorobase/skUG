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

#include <SC_HiddenWorld.h>
#include <SC_PlugIn.h>
#include <SC_Reply.h>
#include <scsynthsend.h>

static InterfaceTable *ft;

struct SendTrigN : public Unit
{
	float m_prevtrig;
    float* m_values;
    
    inline size_t valueOffset() const { return 2; }
    inline size_t numValues() const { return mNumInputs - valueOffset(); }
};

extern "C"
{
	void load(InterfaceTable *inTable);
    void SendTrigN_Ctor(SendTrigN *unit);
    void SendTrigN_next(SendTrigN *unit, int inNumSamples);
};

void SendTrigN_Ctor(SendTrigN *unit)
{
	SETCALC(SendTrigN_next);
	unit->m_prevtrig = 0.f;
    unit->m_values = (float*)RTAlloc(unit->mWorld, unit->numValues()*sizeof(float));
}

void SendTrigN_next(SendTrigN *unit, int inNumSamples)
{
    const char* cmdName = "/tr";

    float *trig = ZIN(0);
    float prevtrig = unit->m_prevtrig;

    if (unit->m_values == 0)
        return;
    
    LOOP(inNumSamples, 
        float curtrig = ZXP(trig);
        if (curtrig > 0.f && prevtrig <= 0.f) {
            for (size_t i=0; i < unit->numValues(); ++i) {
                unit->m_values[i] = ZIN0(unit->valueOffset()+i);
            }
            SendNodeReply(
                &unit->mParent->mNode,
                (int)ZIN0(1),
                cmdName,
                unit->numValues(),
                unit->m_values);
        }
        prevtrig = curtrig;
    );
    unit->m_prevtrig = prevtrig;
}

void load(InterfaceTable *inTable)
{
	ft = inTable;

	DefineSimpleUnit(SendTrigN);
}
