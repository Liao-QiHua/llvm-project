//===-- RV32IMCAsmInfo.h - RV32I asm properties ---------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RV32I_MCTARGETDESC_RV32IMCASMINFO_H
#define LLVM_LIB_TARGET_RV32I_MCTARGETDESC_RV32IMCASMINFO_H

#include "llvm/MC/MCAsmInfoELF.h"

namespace llvm {
class Triple;

class RV32IMCAsmInfo : public MCAsmInfoELF {
  void anchor() override;

public:
  explicit RV32IMCAsmInfo(const Triple &TheTriple,
                          const MCTargetOptions &Options);
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_RV32I_MCTARGETDESC_RV32IMCASMINFO_H
