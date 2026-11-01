//===-- RV32ISubtarget.cpp - RV32I Subtarget Information ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RV32ISubtarget.h"
#include "RV32I.h"

using namespace llvm;

#define DEBUG_TYPE "rv32i-subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "RV32IGenSubtargetInfo.inc"

RV32ISubtarget &RV32ISubtarget::initializeSubtargetDependencies(StringRef CPU,
                                                                StringRef TuneCPU,
                                                                StringRef FS) {
  std::string CPUName = std::string(CPU);
  if (CPUName.empty())
    CPUName = "generic-rv32i";
  std::string TuneCPUName = std::string(TuneCPU);
  if (TuneCPUName.empty())
    TuneCPUName = CPUName;

  ParseSubtargetFeatures(CPUName, TuneCPUName, FS);
  return *this;
}

RV32ISubtarget::RV32ISubtarget(const Triple &TT, StringRef CPU, StringRef TuneCPU,
                               StringRef FS, const TargetMachine &TM)
    : RV32IGenSubtargetInfo(TT, CPU, TuneCPU, FS),
      FrameLowering(initializeSubtargetDependencies(CPU, TuneCPU, FS)),
      InstrInfo(*this), TLInfo(TM, *this) {}
