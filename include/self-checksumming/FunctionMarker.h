#pragma once

#include "llvm/Pass.h"
#include "self-checksumming/FunctionInfo.h"
#include <unordered_set>

namespace llvm {
class Function;
}

//std::unordered_set<llvm::Function*> FunctionInformation::m_functions {};

class FunctionMarkerPass : public llvm::ModulePass {
public:
  static char ID;
public:
  FunctionMarkerPass() : llvm::ModulePass(ID) {}

public:
  bool runOnModule(llvm::Module &M) override;
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  FunctionInformation* get_functions_info();
 // {
   // return FunctionInformation::m_functions_info;
 // }

private:
   FunctionInformation m_functions_info;
}; // class AssertFunctionMarkPass

