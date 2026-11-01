//===-- RV32ISchedule.td - RV32I Scheduling Definitions -----*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Abstract scheduling resources.  Concrete microarchitecture models (which map
// these abstract resources onto real pipeline resources and latencies) are
// added in later patches.
//
//===----------------------------------------------------------------------===//

// Write resources: one per class of result-producing operation.
def WriteIALU  : SchedWrite; // integer ALU: add/sub/and/or/xor/slt
def WriteShift : SchedWrite; // shifts by immediate or register
def WriteLoad  : SchedWrite; // loads
def WriteStore : SchedWrite; // stores (no result, but consumes issue slots)
def WriteJmp   : SchedWrite; // conditional branches
def WriteJal   : SchedWrite; // jal
def WriteJalr  : SchedWrite; // jalr / ret

// Read resources.
def ReadIALU      : SchedRead;
def ReadMemBase   : SchedRead;
def ReadStoreData : SchedRead;

// A simple in-order, single-issue model.  It is good enough to keep the
// scheduler and llvm-mca happy until a real microarchitecture model is added.
def RV32ISchedModel : SchedMachineModel {
  let IssueWidth = 1;
  let MicroOpBufferSize = 0;
  let LoadLatency = 3;
  let MispredictPenalty = 3;
  let CompleteModel = 0;
}
