#include "FunctionInfo.h"

//FunctionInformation
void FunctionInformation::add_function( llvm::Function *F) {
  llvm::dbgs()<<"FunctionInfo. Adding function:"<<F->getName()<<"\n";
  m_functions.insert(F);
}
void FunctionInformation::init(){
  m_functions={};
}
bool FunctionInformation::is_function(llvm::Function *F) const {
   if(m_functions.empty() || !F){
     return false;
   }   
  return m_functions.find(F) !=m_functions.end();
}
const FunctionInformation::FunctionSet &
FunctionInformation::get_functions() const {
  return m_functions;
}

