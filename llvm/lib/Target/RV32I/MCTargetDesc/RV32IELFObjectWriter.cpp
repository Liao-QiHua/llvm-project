//===-- RV32IELFObjectWriter.cpp - RV32I ELF Writer -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// RV32I is instruction-set compatible with RISC-V RV32I, so we emit standard
// RISC-V ELF objects (EM_RISCV) with the R_RISCV_* relocation types.  This
// makes the output directly consumable by RISC-V linkers and objdump.
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/RV32IFixupKinds.h"
#include "MCTargetDesc/RV32IMCTargetDesc.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

namespace {

class RV32IELFObjectWriter : public MCELFObjectTargetWriter {
public:
  explicit RV32IELFObjectWriter(uint8_t OSABI)
      : MCELFObjectTargetWriter(/*Is64Bit=*/false, OSABI, ELF::EM_RISCV,
                                /*HasRelocationAddend=*/true) {}

  ~RV32IELFObjectWriter() override = default;

protected:
  unsigned getRelocType(MCContext &Ctx, const MCValue &Target,
                        const MCFixup &Fixup, bool IsPCRel) const override;
};

} // end anonymous namespace

unsigned RV32IELFObjectWriter::getRelocType(MCContext & /*Ctx*/,
                                            const MCValue & /*Target*/,
                                            const MCFixup &Fixup,
                                            bool /*IsPCRel*/) const {
  switch (Fixup.getKind()) {
  case RV32I::FIXUP_RV32I_BRANCH:
    return ELF::R_RISCV_BRANCH;
  case RV32I::FIXUP_RV32I_JAL:
    return ELF::R_RISCV_JAL;
  case RV32I::FIXUP_RV32I_CALL:
    return ELF::R_RISCV_CALL;
  case RV32I::FIXUP_RV32I_32:
  case FK_Data_4:
    return ELF::R_RISCV_32;
  default:
    llvm_unreachable("RV32I: invalid fixup kind for ELF");
  }
}

std::unique_ptr<MCObjectTargetWriter>
llvm::createRV32IELFObjectWriter(uint8_t OSABI, bool /*Is64Bit*/) {
  return std::make_unique<RV32IELFObjectWriter>(OSABI);
}
