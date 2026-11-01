# 从零移植一个 LLVM 后端：RV32I 增量教程

本目录记录在 LLVM 中**从零新增一个后端**的完整过程。

目标后端叫 `RV32I`：

- 指令集语义与 RISC-V **RV32I** 基本整数指令集等价（同样的 32 位编码、同样的 ABI 寄存器约定）；
- 但它是 LLVM 中的一个**独立目标**：独立 triple `rv32i-...`、独立命名空间、独立 TableGen 描述、独立 MC 层，
  不依赖 `llvm/lib/Target/RISCV`；
- 因此可以把"移植一个新 ISA 后端"的每一步都看清楚，而不会被 RISC-V 后端里大量的扩展代码淹没。

> 一句话：**一个和 RV32I 功能一样、但在 LLVM 里完全"另起炉灶"的后端。**

---

## 1. 分支与构建

```bash
git switch rv32i-backend-port
```

专用最小构建目录（只编译 RV32I 一个目标，避免全量构建）：

```bash
cmake -S llvm -B build-rv32i -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_ENABLE_ASSERTIONS=ON \
  -DLLVM_TARGETS_TO_BUILD=RV32I \
  -DLLVM_INCLUDE_TESTS=OFF -DLLVM_INCLUDE_EXAMPLES=OFF \
  -DLLVM_INCLUDE_BENCHMARKS=OFF -DLLVM_INCLUDE_DOCS=OFF \
  -DLLVM_BUILD_UTILS=OFF -DLLVM_ENABLE_BINDINGS=OFF \
  -DLLVM_ENABLE_TERMINFO=OFF -DLLVM_ENABLE_ZLIB=OFF \
  -DLLVM_ENABLE_ZSTD=OFF -DLLVM_ENABLE_LIBXML2=OFF \
  -DLLVM_ENABLE_CURL=OFF -DLLVM_ENABLE_LIBEDIT=OFF \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++

ninja -C build-rv32i llc
```

验证目标已注册：

```bash
./build-rv32i/bin/llc --version | grep -A1 rv32i
```

---

## 2. 补丁序列（每个补丁都能独立编译）

| # | 补丁 | 新增能力 | 验证方式 |
|---|---|---|---|
| 01 | `[RV32I] Add the target skeleton and lower \`ret void\`` | Triple/CMake 注册、寄存器文件、最小 MC 层、空函数返回 | `llc retvoid.ll` → `ret` |
| 02 | 常量与返回值 | `ADDI`/`LUI`、`li` 物化、返回值搬运 | `ret i32 42` → `li a0, 42; ret` |
| 03 | 参数与整数 ALU | 寄存器分配、`add/sub/and/or/xor/slt/sltu/shift` 及其模式 | `add3(a,b,c)` |
| 04 | 控制流 | 条件分支、比较、无条件跳转、循环 | `if/else`、`while` |
| 05 | 内存与栈帧 | load/store、alloca、frame index、prologue/epilogue、spill | 局部数组、多变量 |
| 06 | 函数调用 | `LowerCall`、`jal`、callee-saved、递归 | `fact(n)` |
| 07 | 目标文件 | MCCodeEmitter、AsmBackend、ELF、重定位 | `-filetype=obj` + `readelf` |
| 08 | 汇编器/反汇编器 | asm matcher、disassembler | `llvm-mc -show-encoding` |
| 09 | 完整化 | 全部 RV32I 指令、调度模型、测试 | 与 `riscv32` 输出对照 |

每个补丁的详细说明见同目录下的 `01-*.md`、`02-*.md` … 文件，包含：

- 改了哪些文件、为什么；
- 关键代码片段；
- 如何复现与预期输出；
- 该步骤在 LLVM 后端流程中的位置。

---

## 3. 这个后端不是什么

- 不是 RISC-V 后端的复制品：没有 M/A/F/D/V 扩展，没有压缩指令，没有 RISC-V 特有的 pass；
- 不追求与 GCC/binutils 完全一致的汇编语法细节（但编码遵循 RV32I，因此生成的目标文件在字节层面与 RV32I 一致）；
- 目标是**可读、可增量复现**，每一层都只引入当时必需的最小实现。

---

## 4. 建议的阅读方式

1. 按补丁顺序读文档，同时在另一个终端 `git show <commit>` 看 diff；
2. 每个补丁先跑"验证方式"里的命令，确认行为；
3. 遇到不理解的点，用 `llc -print-after=<pass>` / `-debug-only=isel` 观察中间结果。

---

## 5. 进度

- [x] 01 骨架与 `ret void`
- [x] 02 常量与返回值
- [x] 03 参数与整数 ALU
- [x] 04 控制流
- [ ] 05 内存与栈帧
- [ ] 06 函数调用
- [ ] 07 目标文件
- [ ] 08 汇编器/反汇编器
- [ ] 09 完整化
