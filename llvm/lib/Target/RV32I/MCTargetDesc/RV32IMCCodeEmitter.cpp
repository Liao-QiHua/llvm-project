//===-- RV32IMCCodeEmitter.cpp - Convert RV32I code to machine code -------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/RV32IFixupKinds.h"
#include "MCTargetDesc/RV32IMCTargetDesc.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include <cassert>
#include <cstdint>

using namespace llvm;

namespace {

class RV32IMCCodeEmitter : public MCCodeEmitter {
public:
  RV32IMCCodeEmitter(const MCInstrInfo &MCII, MCContext & /*Ctx*/)
      : MCII(MCII) {}
  RV32IMCCodeEmitter(const RV32IMCCodeEmitter &) = delete;
  void operator=(const RV32IMCCodeEmitter &) = delete;
  ~RV32IMCCodeEmitter() override = default;

  void encodeInstruction(const MCInst &MI, SmallVectorImpl<char> &CB,
                         SmallVectorImpl<MCFixup> &Fixups,
                         const MCSubtargetInfo &STI) const override;

  // TableGen-generated encoder.
  uint64_t getBinaryCodeForInstr(const MCInst &MI,
                                 SmallVectorImpl<MCFixup> &Fixups,
                                 const MCSubtargetInfo &STI) const;

  // Return the binary encoding of an operand.  Symbolic operands produce a
  // fixup and encode as zero.
  unsigned getMachineOpValue(const MCInst &MI, const MCOperand &MO,
                             SmallVectorImpl<MCFixup> &Fixups,
                             const MCSubtargetInfo &STI) const;

  // Encoder methods used by branch/jump/call target operands.
  unsigned getBranchTargetOpValue(const MCInst &MI, unsigned OpNo,
                                  SmallVectorImpl<MCFixup> &Fixups,
                                  const MCSubtargetInfo &STI) const;
  unsigned getJumpTargetOpValue(const MCInst &MI, unsigned OpNo,
                                SmallVectorImpl<MCFixup> &Fixups,
                                const MCSubtargetInfo &STI) const;
  unsigned getCallTargetOpValue(const MCInst &MI, unsigned OpNo,
                                SmallVectorImpl<MCFixup> &Fixups,
                                const MCSubtargetInfo &STI) const;

private:
  const MCInstrInfo &MCII;
};

} // end anonymous namespace

unsigned RV32IMCCodeEmitter::getMachineOpValue(
    const MCInst &MI, const MCOperand &MO, SmallVectorImpl<MCFixup> &Fixups,
    const MCSubtargetInfo & /*STI*/) const {
  if (MO.isReg()) {
    // The register file is defined so that x0..x31 are consecutive; their
    // hardware encoding is simply their index.
    return static_cast<unsigned>(MO.getReg() - RV32I::X0);
  }

  if (MO.isImm())
    return static_cast<unsigned>(MO.getImm());

  assert(MO.isExpr() && "unexpected operand kind");
  const MCExpr *Expr = MO.getExpr();

  unsigned Kind;
  switch (MI.getOpcode()) {
  case RV32I::PseudoCALL:
    Kind = RV32I::FIXUP_RV32I_CALL;
    break;
  case RV32I::PseudoBR:
  case RV32I::JAL:
    Kind = RV32I::FIXUP_RV32I_JAL;
    break;
  default:
    // Conditional branches.
    Kind = RV32I::FIXUP_RV32I_BRANCH;
    break;
  }

  Fixups.push_back(MCFixup::create(0, Expr, MCFixupKind(Kind)));
  return 0;
}

unsigned RV32IMCCodeEmitter::getBranchTargetOpValue(
    const MCInst &MI, unsigned OpNo, SmallVectorImpl<MCFixup> &Fixups,
    const MCSubtargetInfo & /*STI*/) const {
  const MCOperand &MO = MI.getOperand(OpNo);
  assert(MO.isExpr() && "expected a branch target expression");
  Fixups.push_back(MCFixup::create(
      0, MO.getExpr(), MCFixupKind(RV32I::FIXUP_RV32I_BRANCH)));
  return 0;
}

unsigned RV32IMCCodeEmitter::getJumpTargetOpValue(
    const MCInst &MI, unsigned OpNo, SmallVectorImpl<MCFixup> &Fixups,
    const MCSubtargetInfo & /*STI*/) const {
  const MCOperand &MO = MI.getOperand(OpNo);
  assert(MO.isExpr() && "expected a jump target expression");
  Fixups.push_back(
      MCFixup::create(0, MO.getExpr(), MCFixupKind(RV32I::FIXUP_RV32I_JAL)));
  return 0;
}

unsigned RV32IMCCodeEmitter::getCallTargetOpValue(
    const MCInst &MI, unsigned OpNo, SmallVectorImpl<MCFixup> &Fixups,
    const MCSubtargetInfo & /*STI*/) const {
  const MCOperand &MO = MI.getOperand(OpNo);
  assert(MO.isExpr() && "expected a call target expression");
  Fixups.push_back(
      MCFixup::create(0, MO.getExpr(), MCFixupKind(RV32I::FIXUP_RV32I_CALL)));
  return 0;
}

void RV32IMCCodeEmitter::encodeInstruction(const MCInst &MI,
                                           SmallVectorImpl<char> &CB,
                                           SmallVectorImpl<MCFixup> &Fixups,
                                           const MCSubtargetInfo &STI) const {
  uint64_t Bits = getBinaryCodeForInstr(MI, Fixups, STI);

  // RV32I instructions are 32-bit little-endian.
  for (unsigned I = 0; I != 4; ++I)
    CB.push_back(static_cast<char>((Bits >> (8 * I)) & 0xFF));
}

MCCodeEmitter *llvm::createRV32IMCCodeEmitter(const MCInstrInfo &MCII,
                                              MCContext &Ctx) {
  return new RV32IMCCodeEmitter(MCII, Ctx);
}

#include "RV32IGenMCCodeEmitter.inc"
