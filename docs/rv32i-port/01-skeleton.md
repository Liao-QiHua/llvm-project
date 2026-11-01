# Patch 01：目标骨架与 `ret void`

## 目标

让 LLVM "认识"一个全新的目标 `rv32i`，并完成一条最有价值的最小纵向切片：

```llvm
define void @empty() {
  ret void
}
```

能被 `llc` 编译成：

```asm
empty:
	ret
```

这条切片虽然只产出一条指令，但它强迫我们把后端的**所有层**都打通一次：

| 层 | 本补丁引入的内容 |
|---|---|
| Triple | `Triple::rv32i`，triple 字符串 `rv32i-unknown-elf` |
| 构建系统 | `LLVM_ALL_TARGETS`、`llvm/lib/Target/RV32I/CMakeLists.txt` |
| TableGen 描述 | `RV32I.td`、寄存器文件、调用约定、一条 `RET` 指令 |
| TargetMachine | 数据布局、Subtarget、TLOF、PassConfig |
| SelectionDAG | `RV32ITargetLowering`、`RV32IDAGToDAGISel` |
| MC 层 | MCAsmInfo、InstPrinter、MCTargetDesc 注册 |
| AsmPrinter | `MachineInstr` → `MCInst` → 汇编文本 |

## 改动文件

```
llvm/include/llvm/TargetParser/Triple.h
llvm/lib/TargetParser/Triple.cpp
llvm/CMakeLists.txt
llvm/lib/Target/RV32I/
  CMakeLists.txt
  RV32I.h
  RV32I.td
  RV32ICallingConv.td
  RV32IInstrInfo.td
  RV32IRegisterInfo.td
  RV32ISchedule.td
  RV32IInstrInfo.{h,cpp}
  RV32IRegisterInfo.{h,cpp}
  RV32IFrameLowering.{h,cpp}
  RV32ISubtarget.{h,cpp}
  RV32ITargetMachine.{h,cpp}
  RV32IISelLowering.{h,cpp}
  RV32IISelDAGToDAG.cpp
  RV32IAsmPrinter.cpp
  MCTargetDesc/
    CMakeLists.txt
    RV32IMCTargetDesc.{h,cpp}
    RV32IMCAsmInfo.{h,cpp}
    RV32IInstPrinter.{h,cpp}
  TargetInfo/
    CMakeLists.txt
    RV32ITargetInfo.{h,cpp}
```

## 关键设计点

### 1. 为什么新增一个 Triple arch，而不是复用 `riscv32`

如果复用 `riscv32`，triple 会被路由到已有的 RISC-V 后端，我们的新目标永远拿不到控制权。
`Triple::rv32i` 是路由的起点：`llc -mtriple=rv32i-...` → `TargetRegistry::lookupTarget`

```cpp
// Triple.h
rv32i,          // RV32I: RV32I-compatible 32-bit RISC backend
LastArchType = rv32i
```

同时需要更新 `Triple.cpp` 中的若干 switch：

- `getArchTypeName` → `"rv32i"`
- `getArchTypePrefix` → `"rv32i"`
- `parseArch` / `getArchTypeForLLVMName`
- `getArchPointerBitWidth` → 32
- `get32BitArchVariant`（已是 32 位）
- `get64BitArchVariant`（没有 64 位变体）
- `getBigEndianArchVariant`（没有大端变体）
- `isLittleEndian` → true

### 2. 目标名与命名空间

| 概念 | 值 |
|---|---|
| 目录 | `llvm/lib/Target/RV32I` |
| TableGen `Namespace` | `RV32I` |
| Target 名（`-march` / triple arch） | `rv32i` |
| 生成文件前缀 | `RV32IGen*.inc` |
| C++ 库 | `LLVMRV32ICodeGen`、`LLVMRV32IDesc`、`LLVMRV32IInfo` |

### 3. 寄存器文件

`RV32IRegisterInfo.td` 定义了 x0–x31，并给它们加上 ABI 别名：

```tablegen
class RV32IReg< bits<5> Enc, string n, list<string> alt = []> : Register<n, alt> {
  let HWEncoding{4-0} = Enc;
}

def X10 : RV32IReg<10, "x10", ["a0"]>, DwarfRegNum<[10]>;
...
def GPR : RegisterClass<"RV32I", [i32], 32, (add
    (sequence "X%u", 10, 17), // a0-a7：参数寄存器优先
    (sequence "X%u", 5, 7),   // t0-t2
    (sequence "X%u", 28, 31), // t3-t6
    (sequence "X%u", 8, 9),   // s0-s1
    (sequence "X%u", 18, 27), // s2-s11
    (sequence "X%u", 0, 4)    // 特殊寄存器最后
)>;
```

寄存器类的**顺序就是寄存器分配的首选顺序**，这一点在 patch 03（第一次真正分配寄存器）时开始生效。

### 4. 只定义一条指令

```tablegen
class RVInst<dag outs, dag ins, string asmstr, list<dag> pattern = []>
    : Instruction {
  let Namespace = "RV32I";
  dag OutOperandList = outs;
  dag InOperandList = ins;
  let AsmString = asmstr;
  let Pattern = pattern;
  field bits<32> Inst;
  field bits<32> SoftFail = 0;
  let Size = 4;
}

def RET : RVInst<(outs), (ins), "ret", [(rv32i_ret)]> {
  let Inst = 0x00008067;   // jalr x0, 0(ra)
  let isReturn = 1;
  let isBarrier = 1;
  let isTerminator = 1;
}
```

`Pattern` 中的 `rv32i_ret` 是一个 RISC-V 后端同款技巧：把"函数返回"建模成一个带 chain 的私有 DAG 节点，
再由 TableGen 生成匹配表把它选成 `RET`。

### 5. 最小 lowering

```cpp
SDValue RV32ITargetLowering::LowerReturn(
    SDValue Chain, CallingConv::ID, bool IsVarArg,
    const SmallVectorImpl<ISD::OutputArg> &Outs,
    const SmallVectorImpl<SDValue> &, const SDLoc &DL,
    SelectionDAG &DAG) const {
  if (IsVarArg)
    report_fatal_error("RV32I: varargs are not supported yet");
  if (!Outs.empty())
    report_fatal_error("RV32I: return values are implemented in a later patch");

  return DAG.getNode(RV32IISD::RET, DL, MVT::Other, Chain);
}
```

这是刻意的"垂直切片"：先只支持空返回，后续补丁按需扩展，而不是一次性写完。

### 6. 为什么现在就要 MC 层

因为 `LLVMTargetMachine` 的构造函数会调用 `initAsmInfo()`，它要求目标已经注册：

- `MCInstrInfo`
- `MCRegisterInfo`
- `MCSubtargetInfo`
- `MCAsmInfo`
- `MCInstPrinter`（`-filetype=asm` 时）
- `AsmPrinter`（`LLVMInitializeRV32IAsmPrinter`）

所以哪怕只有一条指令，MC 层的注册也必须完整。

## 复现

```bash
ninja -C build-rv32i llc

cat > /tmp/retvoid.ll <<'EOF'
define void @empty() {
  ret void
}
EOF

./build-rv32i/bin/llc -mtriple=rv32i-unknown-elf /tmp/retvoid.ll -o -
```

预期输出：

```asm
	.text
	.file	"retvoid.ll"
	.globl	empty
	.p2align	2
	.type	empty,@function
empty:
	ret
.Lfunc_end0:
	.size	empty, .Lfunc_end0-empty
```

## 观察点

```bash
# 1. 目标已被 LLVM 注册
./build-rv32i/bin/llc --version | grep rv32i

# 2. 指令选择的 DAG（需要 assertions 构建）
./build-rv32i/bin/llc -mtriple=rv32i /tmp/retvoid.ll -debug-only=isel -o /dev/null

# 3. 选择后的 MachineInstr
./build-rv32i/bin/llc -mtriple=rv32i /tmp/retvoid.ll -print-after=finalize-isel -o /dev/null
```

## 本补丁刻意留下的缺口

这些缺口会在后续补丁中逐个补上，属于教学设计而非缺陷：

- 不支持返回值、参数、算术、分支、访存、调用；
- `copyPhysReg` / spill / frame index 都是 `llvm_unreachable`；
- 没有 MCCodeEmitter / AsmBackend / ELF writer，因此还不支持 `-filetype=obj`；
- 没有汇编器与反汇编器。
