//===-- RV32IAsmPrinter.cpp - RV32I LLVM assembly writer ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/RV32IMCTargetDesc.h"
#include "RV32IInstrInfo.h"
#include "RV32ITargetMachine.h"
#include "TargetInfo/RV32ITargetInfo.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

namespace {

class RV32IAsmPrinter : public AsmPrinter {
public:
  RV32IAsmPrinter(TargetMachine &TM, std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer)) {}

  StringRef getPassName() const override { return "RV32I Assembly Printer"; }

  void emitInstruction(const MachineInstr *MI) override;

private:
  MCOperand lowerOperand(const MachineOperand &MO);
};

} // end anonymous namespace

MCOperand RV32IAsmPrinter::lowerOperand(const MachineOperand &MO) {
  switch (MO.getType()) {
  case MachineOperand::MO_Register:
    // Implicit operands (e.g. the implicit def of ra for a call) are not part
    // of the instruction encoding.
    return MCOperand::createReg(MO.getReg());
  case MachineOperand::MO_Immediate:
    return MCOperand::createImm(MO.getImm());
  case MachineOperand::MO_MachineBasicBlock:
    return MCOperand::createExpr(
        MCSymbolRefExpr::create(MO.getMBB()->getSymbol(), OutContext));
  case MachineOperand::MO_ExternalSymbol:
    return MCOperand::createExpr(
        MCSymbolRefExpr::create(GetExternalSymbolSymbol(MO.getSymbolName()),
                                OutContext));
  case MachineOperand::MO_GlobalAddress:
    return MCOperand::createExpr(
        MCSymbolRefExpr::create(getSymbol(MO.getGlobal()), OutContext));
  default:
    llvm_unreachable("RV32I: unsupported machine operand kind");
  }
}

void RV32IAsmPrinter::emitInstruction(const MachineInstr *MI) {
  MCInst TmpInst;
  TmpInst.setOpcode(MI->getOpcode());
  for (const MachineOperand &MO : MI->explicit_operands())
    TmpInst.addOperand(lowerOperand(MO));

  EmitToStreamer(*OutStreamer, TmpInst);
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeRV32IAsmPrinter() {
  RegisterAsmPrinter<RV32IAsmPrinter> X(getTheRV32ITarget());
}
