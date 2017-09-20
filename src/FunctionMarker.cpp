#include "FunctionMarker.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include <fstream>
using namespace llvm;

//FunctionInformation
void FunctionInformation::add_function( llvm::Function *F) {
  //dbgs()<<"In insert\n";
  m_functions.insert(F);
}
void FunctionInformation::init(){
  m_functions={};
}
bool FunctionInformation::is_function(llvm::Function *F) const {
   //dbgs()<<"In is_function\n";
   if(m_functions.size()==0 || !F){
     return false;
   } 
   //dbgs()<<F->getName()<<" "<<m_functions.size()<<"\n";
  return m_functions.find(F) !=m_functions.end();
}
const FunctionInformation::FunctionSet &
FunctionInformation::get_functions() const {
  //dbgs()<<"In get_functions\n";
  //dbgs()<<m_functions.size()<<"\n";
  return m_functions;
}

//FunctionMarkerPass
void FunctionMarkerPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.setPreservesAll();
}
char FunctionMarkerPass::ID = 0;

bool FunctionMarkerPass::runOnModule(llvm::Module &M) {
  llvm::dbgs() << "In  mark function pass "<< "\n";
  this->m_functions_info.init();
  return false;
}
FunctionInformation* FunctionMarkerPass::get_functions_info() {
  return &(this->m_functions_info);
}
static llvm::RegisterPass<FunctionMarkerPass>
    X("marker-func", "Marks functions in a given file as assert functions");
