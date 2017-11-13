#include "FunctionInfo.h"

//FunctionInformation
void FunctionInformation::add_function( llvm::Function *F) {
  m_functions.insert(F);
}
void FunctionInformation::init(){
  m_functions={};
}
bool FunctionInformation::is_function(llvm::Function *F) const {
   if(m_functions.size()==0 || !F){
     return false;
   }   
  return m_functions.find(F) !=m_functions.end();
}
const FunctionInformation::FunctionSet &
FunctionInformation::get_functions() const {
  return m_functions;
}

