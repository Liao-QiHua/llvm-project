//===-- RV32IMCTargetDesc.h - RV32I Target Descriptions ---------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file provides RV32I specific target descriptions (MC layer).
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RV32I_MCTARGETDESC_RV32IMCTARGETDESC_H
#define LLVM_LIB_TARGET_RV32I_MCTARGETDESC_RV32IMCTARGETDESC_H

#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/Support/DataTypes.h"

namespace llvm {
class MCCodeEmitter;
class MCContext;
class MCInstrInfo;
class MCSubtargetInfo;
class Target;

} // namespace llvm

// Defines symbolic names for RV32I registers.
#define GET_REGINFO_ENUM
#include "RV32IGenRegisterInfo.inc"

// Defines symbolic names for the RV32I instructions.
#define GET_INSTRINFO_ENUM
#define GET_INSTRINFO_MC_HELPER_DECLS
#include "RV32IGenInstrInfo.inc"

// Defines symbolic names for the RV32I subtarget features.
#define GET_SUBTARGETINFO_ENUM
#include "RV32IGenSubtargetInfo.inc"

#endif // LLVM_LIB_TARGET_RV32I_MCTARGETDESC_RV32IMCTARGETDESC_H
