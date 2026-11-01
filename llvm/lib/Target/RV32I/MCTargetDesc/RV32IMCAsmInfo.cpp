//===-- RV32IMCAsmInfo.cpp - RV32I asm properties -------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RV32IMCAsmInfo.h"
#include "llvm/TargetParser/Triple.h"

using namespace llvm;

void RV32IMCAsmInfo::anchor() {}

RV32IMCAsmInfo::RV32IMCAsmInfo(const Triple & /*TheTriple*/,
                               const MCTargetOptions & /*Options*/) {
  IsLittleEndian = true;
  CodePointerSize = 4;
  CalleeSaveStackSlotSize = 4;

  PrivateGlobalPrefix = ".L";
  CommentString = "#";
  AlignmentIsInBytes = false;
  UsesELFSectionDirectiveForBSS = true;
  SupportsDebugInformation = true;
  ExceptionsType = ExceptionHandling::DwarfCFI;
  MinInstAlignment = 4;

  // The integrated assembler is added in a later patch; until then, do not
  // try to parse inline assembly with a (non-existent) target parser.
  UseIntegratedAssembler = false;
}
