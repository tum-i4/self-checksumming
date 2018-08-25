#pragma once

#include "llvm/Pass.h"
#include "self-checksumming/FunctionInfo.h"
#include <unordered_set>

namespace llvm {
class Function;
}


class FunctionFilterPass : public llvm::ModulePass {
public:
  static char ID;
public:
  FunctionFilterPass() : llvm::ModulePass(ID) {}

public:
  bool runOnModule(llvm::Module &M) override;
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  FunctionInformation* get_functions_info();
  void loadFile(llvm::Module &M, std::string file_name);

private:
   FunctionInformation m_functions_info;
}; 

