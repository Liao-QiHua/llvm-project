add_llvm_component_library(LLVMRV32IDesc
  RV32IInstPrinter.cpp
  RV32IMCAsmInfo.cpp
  RV32IMCTargetDesc.cpp

  LINK_COMPONENTS
  MC
  RV32IInfo
  Support
  TargetParser

  ADD_TO_COMPONENT
  RV32I
  )
