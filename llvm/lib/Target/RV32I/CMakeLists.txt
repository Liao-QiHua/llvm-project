add_llvm_component_group(RV32I)

set(LLVM_TARGET_DEFINITIONS RV32I.td)

tablegen(LLVM RV32IGenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM RV32IGenDAGISel.inc -gen-dag-isel)
tablegen(LLVM RV32IGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM RV32IGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM RV32IGenSubtargetInfo.inc -gen-subtarget)

add_public_tablegen_target(RV32ICommonTableGen)

add_llvm_target(RV32ICodeGen
  RV32IAsmPrinter.cpp
  RV32IFrameLowering.cpp
  RV32IInstrInfo.cpp
  RV32IISelDAGToDAG.cpp
  RV32IISelLowering.cpp
  RV32IRegisterInfo.cpp
  RV32ISubtarget.cpp
  RV32ITargetMachine.cpp

  LINK_COMPONENTS
  Analysis
  AsmPrinter
  CodeGen
  CodeGenTypes
  Core
  MC
  RV32IDesc
  RV32IInfo
  SelectionDAG
  Support
  Target
  TargetParser
  TransformUtils

  ADD_TO_COMPONENT
  RV32I
  )

add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)
