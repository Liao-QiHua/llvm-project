//===-- RV32IFixupKinds.h - RV32I Specific Fixup Entries --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RV32I_MCTARGETDESC_RV32IFIXUPKINDS_H
#define LLVM_LIB_TARGET_RV32I_MCTARGETDESC_RV32IFIXUPKINDS_H

#include "llvm/MC/MCFixup.h"

namespace llvm {
namespace RV32I {

// This table must stay in sync with
// MCFixupKindInfo Infos[] in RV32IAsmBackend.cpp.
enum Fixups {
  FIXUP_RV32I_NONE = FirstTargetFixupKind,

  // 13-bit PC-relative branch offset (B-type).
  FIXUP_RV32I_BRANCH,
  // 21-bit PC-relative jump offset (J-type).
  FIXUP_RV32I_JAL,
  // Direct call: jal ra, symbol.
  FIXUP_RV32I_CALL,
  // 32-bit absolute address.
  FIXUP_RV32I_32,

  LastTargetFixupKind,
  NumTargetFixupKinds = LastTargetFixupKind - FirstTargetFixupKind
};

} // namespace RV32I
} // namespace llvm

#endif // LLVM_LIB_TARGET_RV32I_MCTARGETDESC_RV32IFIXUPKINDS_H
