//===-- RV32IInstPrinter.cpp - Convert RV32I MCInst to asm syntax ---------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RV32IInstPrinter.h"
#include "RV32IMCTargetDesc.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

// Include the auto-generated portion of the assembly writer.
#include "RV32IGenAsmWriter.inc"

// Print the ABI name (a0, sp, ...) instead of the architectural name (x10,
// x2, ...).  The ABI names are what humans expect to see in RV32I assembly.
static const char *getRegisterABIName(MCRegister Reg) {
  static const char *const Names[] = {
      "zero", "ra", "sp",  "gp",  "tp",  "t0",  "t1",  "t2",
      "s0",   "s1", "a0",  "a1",  "a2",  "a3",  "a4",  "a5",
      "a6",   "a7", "s2",  "s3",  "s4",  "s5",  "s6",  "s7",
      "s8",   "s9", "s10", "s11", "t3",  "t4",  "t5",  "t6"};
  if (Reg < RV32I::X0 || Reg > RV32I::X31)
    return nullptr;
  return Names[Reg - RV32I::X0];
}

void RV32IInstPrinter::printInst(const MCInst *MI, uint64_t Address,
                                 StringRef Annot, const MCSubtargetInfo & /*STI*/,
                                 raw_ostream &O) {
  printInstruction(MI, Address, O);
  printAnnotation(O, Annot);
}

void RV32IInstPrinter::printOperand(const MCInst *MI, unsigned OpNo,
                                    raw_ostream &O, const char * /*Modifier*/) {
  const MCOperand &MO = MI->getOperand(OpNo);

  if (MO.isReg()) {
    if (const char *ABIName = getRegisterABIName(MO.getReg()))
      O << ABIName;
    else
      O << getRegisterName(MO.getReg());
  } else if (MO.isImm()) {
    O << MO.getImm();
  } else if (MO.isExpr()) {
    MO.getExpr()->print(O, &MAI);
  } else {
    O << "<invalid operand>";
  }
}

void RV32IInstPrinter::printRegName(raw_ostream &OS, MCRegister Reg) const {
  if (const char *ABIName = getRegisterABIName(Reg))
    OS << ABIName;
  else
    OS << getRegisterName(Reg);
}
