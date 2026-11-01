//===-- RV32IInstPrinter.cpp - Convert RV32I MCInst to asm syntax ---------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RV32IInstPrinter.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

// Include the auto-generated portion of the assembly writer.
#include "RV32IGenAsmWriter.inc"

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
  OS << getRegisterName(Reg);
}
