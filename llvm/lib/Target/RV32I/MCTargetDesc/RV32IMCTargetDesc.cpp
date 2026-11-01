//===-- RV32IMCTargetDesc.cpp - RV32I Target Descriptions -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RV32IMCTargetDesc.h"
#include "RV32IInstPrinter.h"
#include "RV32IMCAsmInfo.h"
#include "TargetInfo/RV32ITargetInfo.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/TargetParser/Triple.h"

#define GET_INSTRINFO_MC_DESC
#include "RV32IGenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "RV32IGenSubtargetInfo.inc"

#define GET_REGINFO_MC_DESC
#include "RV32IGenRegisterInfo.inc"

using namespace llvm;

static MCInstrInfo *createRV32IMCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitRV32IMCInstrInfo(X);
  return X;
}

static MCRegisterInfo *createRV32IMCRegisterInfo(const Triple & /*TT*/) {
  MCRegisterInfo *X = new MCRegisterInfo();
  // The return address register is ra (x1).
  InitRV32IMCRegisterInfo(X, RV32I::X1);
  return X;
}

static MCSubtargetInfo *createRV32IMCSubtargetInfo(const Triple &TT,
                                                   StringRef CPU, StringRef FS) {
  std::string CPUName = std::string(CPU);
  if (CPUName.empty())
    CPUName = "generic-rv32i";
  return createRV32IMCSubtargetInfoImpl(TT, CPUName, /*TuneCPU=*/CPUName, FS);
}

static MCAsmInfo *createRV32IMCAsmInfo(const MCRegisterInfo & /*MRI*/,
                                       const Triple &TT,
                                       const MCTargetOptions &Options) {
  return new RV32IMCAsmInfo(TT, Options);
}

static MCInstPrinter *createRV32IMCInstPrinter(const Triple & /*T*/,
                                               unsigned SyntaxVariant,
                                               const MCAsmInfo &MAI,
                                               const MCInstrInfo &MII,
                                               const MCRegisterInfo &MRI) {
  if (SyntaxVariant == 0)
    return new RV32IInstPrinter(MAI, MII, MRI);
  return nullptr;
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeRV32ITargetMC() {
  RegisterMCAsmInfo<RV32IMCAsmInfo> X(getTheRV32ITarget());

  TargetRegistry::RegisterMCInstrInfo(getTheRV32ITarget(),
                                      createRV32IMCInstrInfo);
  TargetRegistry::RegisterMCRegInfo(getTheRV32ITarget(),
                                    createRV32IMCRegisterInfo);
  TargetRegistry::RegisterMCSubtargetInfo(getTheRV32ITarget(),
                                          createRV32IMCSubtargetInfo);
  TargetRegistry::RegisterMCInstPrinter(getTheRV32ITarget(),
                                        createRV32IMCInstPrinter);
}
