#ifndef FUNCTIONINFORMATION_H
#define FUNCTIONINFORMATION_H

#include "llvm/IR/Function.h"
#include <unordered_set>
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

class FunctionInformation {
public:
  using FunctionSet = std::unordered_set<llvm::Function*>;


public:
  void add_function( llvm::Function *F) ;
  bool is_function(llvm::Function *F) const;
  const FunctionSet &get_functions() const;
  void init();
private:
   FunctionSet m_functions;
}; // class SensitiveFunctionInformation
#endif 
