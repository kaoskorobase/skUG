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

#include <fftw3.h>
#include <math.h>
#include <sndfile.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "VEP.h"
#include "VEPConv.h"
#include "VEPDSP.h"

#include "clz.h"
#include "SC_PlugIn.h"

static InterfaceTable* ft;

namespace VEP
{
  void print(float *src, size_t size, const char *tag)
  {
    printf("%s: %d\n", tag, size);
    for (size_t i=0; i < size; ++i) {
      printf("%+02.6f\n", src[i]);
    }
    printf("-----------------\n");
  }

  // inline SndBuf* getBuffer(World* world, float fbufnum, int channels, int frames)
  // {
  //   uint32 bufnum = (int)fbufnum;
  //   if (bufnum >= world->mNumSndBufs) bufnum = 0;
  //   SndBuf* buf = world->mSndBufs + bufnum;
  //   return ((buf->data == 0)
  //           || (buf->channels != channels)
  //           || (buf->frames < frames)) ? 0 : buf;
  // }
};

struct VEPConvolution : public Unit
{
  enum
  {
    idx_kernel,         // kernel buffer number
    idx_kernelMaxSize,  // max kernel size
    idx_kernelTrigger,  // kernel switch trigger
    idx_kernelOffset,   // kernel frame offset
    idx_kernelSize,     // kernel frame count
    idx_minPartSize,    // minimum partition size
    idx_maxPartSize,    // maximum partition size
    idx_numRTProcs,     // number of convolvers in RT thread
    kNumFixedInputs
  };

  struct Cmd
  {
    enum Type
    {
      kInit,
      kRelease,
      kSetKernel
    };
    struct InitData
    {
      int               numRTProcs;
      VEP::Convolution* conv;
    };
    struct ReleaseData
    {
      VEP::Convolution* conv;
    };
    struct SetKernelData
    {
      int               bufnum;
      int               offset;
      int               length;
    };
    union Data
    {
      InitData          Init;
      SetKernelData     SetKernel;
      ReleaseData       Release;
    };

    VEPConvolution*     unit;
    Type                type;
    Data                data;
    
  };

  bool setKernel(int bufnum, int offset, int length, bool defer);
  void process(size_t numSamples);

  Cmd* allocCmd(Cmd::Type type);
  void doCmd(Cmd* cmd);
  
  static bool cmdStage2(World*, Cmd*);    // NRT
  static bool cmdStage3(World*, Cmd*);    // RT
  static bool cmdStage4(World*, Cmd*);    // NRT
  static void cmdCleanup(World*, void*);  // RT
  
  size_t                m_numChannels;
  size_t                m_kernelMaxSize;
  size_t                m_minPartSize;
  size_t                m_maxPartSize;
  size_t                m_numRTProcs;
  float                 m_bufnum;
  float                 m_buftrig;
  VEP::Convolution*     m_conv;
#if VEP_BENCHMARK
  VEP::PeriodicBenchmark  m_bench;
#endif

};

extern "C"
{
  void VEPConvolution_next(VEPConvolution*, int);
  void VEPConvolution_Ctor(VEPConvolution*);
  void VEPConvolution_Dtor(VEPConvolution*);
  void load(InterfaceTable*);
};

// =====================================================================
// VEPConvolution

#define VEPCONV_IN(i)   (unit->mInBuf[(i) + unit->m_numChannels])
#define VEPCONV_IN0(i)  (VEPCONV_IN(i)[0])

void VEPConvolution_Ctor(VEPConvolution *unit)
{
  //    Print("VEPConvolution_Ctor >>>\n");

  unit->m_numChannels = unit->mNumInputs - VEPConvolution::kNumFixedInputs;
  if (unit->m_numChannels != unit->mNumOutputs) {
    Print("VEPConvolution: I/O channel count mismatch\n");
  }
  unit->m_kernelMaxSize = std::max(0, (int)VEPCONV_IN0(VEPConvolution::idx_kernelMaxSize));
  if (unit->m_kernelMaxSize == 0) {
    int bufnum = (int)VEPCONV_IN0(VEPConvolution::idx_kernel);
    SndBuf* buf = World_GetBuf(unit->mWorld, bufnum);
    if (buf && buf->data) {
      unit->m_kernelMaxSize = buf->frames;
    }
  }
  
  int minPartSize = (int)VEPCONV_IN0(VEPConvolution::idx_minPartSize);
  minPartSize = std::max(16, NEXTPOWEROFTWO(minPartSize > 0 ? minPartSize : BUFLENGTH));
  
  int maxPartSize = NEXTPOWEROFTWO(std::max(minPartSize, (int)VEPCONV_IN0(VEPConvolution::idx_maxPartSize)));

  unit->m_minPartSize = minPartSize;
  unit->m_maxPartSize = maxPartSize;

  unit->m_bufnum = -1e9f;
  unit->m_buftrig = 0.f;
  unit->m_conv = 0;
  
#if VEP_BENCHMARK
  unit->m_bench.init(690);
#endif // VEP_BENCHMARK

  SETCALC(VEPConvolution_next);
  //ClearUnitOutputs(unit, 1);

  VEPConvolution::Cmd* cmd = unit->allocCmd(VEPConvolution::Cmd::kInit);
  cmd->data.Init.numRTProcs =
    unit->mWorld->mRealTime
      ? std::max(0, (int)VEPCONV_IN0(VEPConvolution::idx_numRTProcs))
      : /* no threading in NRT */ 0;
  unit->doCmd(cmd);
  
  //    Print("<<< VEPConvolution_Ctor\n");
}

void VEPConvolution_Dtor(VEPConvolution *unit)
{
  if (unit->m_conv != 0) {
    VEPConvolution::Cmd* cmd = unit->allocCmd(VEPConvolution::Cmd::kRelease);
    cmd->data.Release.conv = unit->m_conv;
    unit->m_conv = 0;
    unit->doCmd(cmd);
  }
}

void VEPConvolution_next(VEPConvolution *unit, int inNumSamples)
{
  //    Print("VEPConvolution_next >>>\n");

  float bufnum = VEPCONV_IN0(VEPConvolution::idx_kernel);
  float buftrig = VEPCONV_IN0(VEPConvolution::idx_kernelTrigger);

  if ((bufnum != unit->m_bufnum) || ((unit->m_buftrig <= 0.f) && (buftrig > 0.f))) {
    unit->m_bufnum = bufnum;
    int kernelOffset = (int)VEPCONV_IN0(VEPConvolution::idx_kernelOffset);
    int kernelSize = (int)VEPCONV_IN0(VEPConvolution::idx_kernelSize);
    unit->setKernel((int)bufnum, kernelOffset, kernelSize, true);
  }
  unit->m_buftrig = buftrig;
  
  if (unit->m_conv && (unit->m_numChannels == unit->mNumOutputs)) {
#ifndef NDEBUG
    for (size_t c=0; c < unit->m_numChannels; ++c)
    {
      assert( unit->mInBuf[c] != unit->mOutBuf[c] );
    }
#endif // !NDEBUG
    unit->process((size_t)inNumSamples);
  } else {
    ClearUnitOutputs(unit, inNumSamples);
  }
  
#if VEP_BENCHMARK
  unit->m_bench.inc();
  unit->m_bench.printSummary(stdout, "conv");
#endif // VEP_BENCHMARK

  //    Print("<<< VEPConvolution_next\n");
}

// =====================================================================
// VEPConvolution::Cmd

VEPConvolution::Cmd* VEPConvolution::allocCmd(Cmd::Type type)
{
  Cmd* cmd = (Cmd*)RTAlloc(mWorld, sizeof(Cmd));
  if (cmd == 0) return 0;
  memset(cmd, 0, sizeof(Cmd));
  cmd->unit = this;
  cmd->type = type;
  return cmd;
}

void VEPConvolution::doCmd(Cmd* cmd)
{
  DoAsynchronousCommand(mWorld, 0, "", (void*)cmd,
                        (AsyncStageFn)cmdStage2,
                        (AsyncStageFn)cmdStage3,
                        (AsyncStageFn)cmdStage4,
                        cmdCleanup,
                        0, 0);
}

bool VEPConvolution::setKernel(int bufnum, int offset, int length, bool defer)
{
  if (m_conv != 0) {
    // do the football
    SndBuf* buf = World_GetBuf(mWorld, bufnum);
    if (buf->data == 0) {
      Print("VEPConvolution: invalid buffer %d\n", bufnum);
      return false;
    }
    // if (buf->channels < unit->m_numChannels) {
    //   Print("VEPConvolution: channel count mismatch for buffer %d\n", data.bufnum);
    //   return false;
    // }
    // TODO: implement offset and size
    m_conv->setKernel(buf->data, buf->channels, buf->frames);
  } else if (defer) {
    // defer
    Cmd* cmd = allocCmd(Cmd::kSetKernel);
    Cmd::SetKernelData& data = cmd->data.SetKernel;
    data.bufnum = bufnum;
    data.offset = offset;
    data.length = length;
    doCmd(cmd);
    return true;
  }
  return false;
}

void VEPConvolution::process(size_t numSamples)
{
  m_conv->process(mOutBuf, const_cast<const float**>(mInBuf), m_numChannels, numSamples);
}

bool VEPConvolution::cmdStage2(World* inWorld, Cmd* cmd) // NRT
{
  switch (cmd->type) {
    case Cmd::kInit: {
      VEPConvolution* unit = cmd->unit;
      VEP::Response* response = new VEP::Response(unit->m_numChannels, unit->m_kernelMaxSize, unit->m_minPartSize, unit->m_maxPartSize);
      cmd->data.Init.conv = new VEP::Convolution(
        VEP::Response(unit->m_numChannels, unit->m_kernelMaxSize, unit->m_minPartSize, unit->m_maxPartSize),
        cmd->data.Init.numRTProcs);
      cmd->data.Init.conv->response().printOn(stdout);
    }
    return true;
    case Cmd::kSetKernel:
    // NOP
    return true;
    case Cmd::kRelease: {
      delete cmd->data.Release.conv;
      cmd->data.Release.conv = 0;
    }
    return true;
  }
  
  return false;
}

bool VEPConvolution::cmdStage3(World* world, Cmd* cmd) // RT
{
  switch (cmd->type) {
    case Cmd::kInit: {
      cmd->unit->m_conv = cmd->data.Init.conv;
    }
    return true;
    case Cmd::kSetKernel: {
      Cmd::SetKernelData& data = cmd->data.SetKernel;
      return cmd->unit->setKernel(data.bufnum, data.offset, data.length, false);
    }
  }
  return true;
}

bool VEPConvolution::cmdStage4(World* world, Cmd* cmd) // NRT
{
  return true;
}

void VEPConvolution::cmdCleanup(World* world, void* cmd)
{
  RTFree(world, cmd);
}

void load(InterfaceTable *it)
{
  ft = it;
  DefineDtorCantAliasUnit(VEPConvolution);
}

// EOF
