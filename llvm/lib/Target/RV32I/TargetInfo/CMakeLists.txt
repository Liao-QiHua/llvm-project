add_llvm_component_library(LLVMRV32IInfo
  RV32ITargetInfo.cpp

  LINK_COMPONENTS
  MC
  Support

  ADD_TO_COMPONENT
  RV32I
  )
