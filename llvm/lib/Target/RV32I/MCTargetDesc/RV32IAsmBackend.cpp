//===-- RV32IAsmBackend.cpp - RV32I Assembler Backend ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/RV32IFixupKinds.h"
#include "MCTargetDesc/RV32IMCTargetDesc.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCFixupKindInfo.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

static uint32_t readLE32(const char *P) {
  return static_cast<uint8_t>(P[0]) |
         (static_cast<uint32_t>(static_cast<uint8_t>(P[1])) << 8) |
         (static_cast<uint32_t>(static_cast<uint8_t>(P[2])) << 16) |
         (static_cast<uint32_t>(static_cast<uint8_t>(P[3])) << 24);
}

static void writeLE32(char *P, uint32_t V) {
  P[0] = static_cast<char>(V & 0xFF);
  P[1] = static_cast<char>((V >> 8) & 0xFF);
  P[2] = static_cast<char>((V >> 16) & 0xFF);
  P[3] = static_cast<char>((V >> 24) & 0xFF);
}

namespace {

class RV32IAsmBackend : public MCAsmBackend {
public:
  RV32IAsmBackend() : MCAsmBackend(support::little) {}

  std::unique_ptr<MCObjectTargetWriter>
  createObjectTargetWriter() const override {
    return createRV32IELFObjectWriter(/*OSABI=*/0, /*Is64Bit=*/false);
  }

  void applyFixup(const MCAssembler &Asm, const MCFixup &Fixup,
                  const MCValue &Target, MutableArrayRef<char> Data,
                  uint64_t Value, bool IsResolved,
                  const MCSubtargetInfo *STI) const override;

  const MCFixupKindInfo &getFixupKindInfo(MCFixupKind Kind) const override {
    static const MCFixupKindInfo Infos[RV32I::NumTargetFixupKinds] = {
        {"FIXUP_RV32I_NONE", 0, 0, 0},
        {"FIXUP_RV32I_BRANCH", 0, 32, MCFixupKindInfo::FKF_IsPCRel},
        {"FIXUP_RV32I_JAL", 0, 32, MCFixupKindInfo::FKF_IsPCRel},
        {"FIXUP_RV32I_CALL", 0, 32, MCFixupKindInfo::FKF_IsPCRel},
        {"FIXUP_RV32I_32", 0, 32, 0},
    };

    if (Kind < FirstTargetFixupKind)
      return MCAsmBackend::getFixupKindInfo(Kind);
    return Infos[Kind - FirstTargetFixupKind];
  }

  unsigned getNumFixupKinds() const override {
    return RV32I::NumTargetFixupKinds;
  }

  bool fixupNeedsRelaxation(const MCFixup & /*Fixup*/, uint64_t /*Value*/,
                            const MCRelaxableFragment * /*DF*/,
                            const MCAsmLayout & /*Layout*/) const override {
    return false;
  }

  bool writeNopData(raw_ostream &OS, uint64_t Count,
                    const MCSubtargetInfo * /*STI*/) const override {
    if (Count % 4 != 0)
      return false;

    // addi x0, x0, 0
    const char Nop[4] = {0x13, 0x00, 0x00, 0x00};
    for (uint64_t I = 0; I != Count; I += 4)
      OS.write(Nop, 4);
    return true;
  }
};

} // end anonymous namespace

void RV32IAsmBackend::applyFixup(const MCAssembler & /*Asm*/,
                                 const MCFixup &Fixup,
                                 const MCValue & /*Target*/,
                                 MutableArrayRef<char> Data, uint64_t Value,
                                 bool /*IsResolved*/,
                                 const MCSubtargetInfo * /*STI*/) const {
  MCFixupKind Kind = Fixup.getKind();
  if (Kind < FirstTargetFixupKind)
    return;

  char *P = Data.data() + Fixup.getOffset();
  uint32_t Inst = readLE32(P);
  const int64_t Imm = static_cast<int64_t>(Value);

  switch (Kind) {
  case RV32I::FIXUP_RV32I_BRANCH: {
    // B-type: imm[12|10:5] rs2 rs1 funct3 imm[4:1|11] opcode
    uint32_t Enc = ((Imm >> 12) & 0x1) << 31;
    Enc |= ((Imm >> 5) & 0x3F) << 25;
    Enc |= ((Imm >> 1) & 0xF) << 8;
    Enc |= ((Imm >> 11) & 0x1) << 7;

    const uint32_t Mask = (0x1u << 31) | (0x3Fu << 25) | (0xFu << 8) | (0x1u << 7);
    Inst = (Inst & ~Mask) | Enc;
    break;
  }
  case RV32I::FIXUP_RV32I_JAL:
  case RV32I::FIXUP_RV32I_CALL: {
    // J-type: imm[20|10:1|11|19:12] rd opcode
    uint32_t Enc = ((Imm >> 20) & 0x1) << 31;
    Enc |= ((Imm >> 1) & 0x3FF) << 21;
    Enc |= ((Imm >> 11) & 0x1) << 20;
    Enc |= ((Imm >> 12) & 0xFF) << 12;

    Inst = (Inst & 0xFFFu) | Enc;
    break;
  }
  case RV32I::FIXUP_RV32I_32:
    Inst = static_cast<uint32_t>(Value);
    break;
  default:
    return;
  }

  writeLE32(P, Inst);
}

MCAsmBackend *llvm::createRV32IAsmBackend(const Target & /*T*/,
                                          const MCSubtargetInfo & /*STI*/,
                                          const MCRegisterInfo & /*MRI*/,
                                          const MCTargetOptions & /*Options*/) {
  return new RV32IAsmBackend();
}
