#pragma once

#include "llvm/Pass.h"

#include <unordered_set>

namespace llvm {
class Function;
}

class FunctionInformation {
public:
  using FunctionSet = std::unordered_set<llvm::Function*>;


public:
//  FunctionInformation() = default;
//  FunctionInformation(const FunctionInformation &) = delete;
//  FunctionInformation &
//  operator=(const FunctionInformation &) = delete;
  void add_function( llvm::Function *F) ;
  bool is_function(llvm::Function *F) const;
  const FunctionSet &get_functions() const;
  void init();
private:
   FunctionSet m_functions;
}; // class SensitiveFunctionInformation

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

