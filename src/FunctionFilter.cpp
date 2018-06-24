#include "FunctionFilter.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include <fstream>
#include <cxxabi.h>
using namespace llvm;


static cl::opt<std::string> FilterFile(
                    "filter-file", cl::Hidden,
                        cl::desc("Path to function filter file"));

//FunctionFilterPass
void FunctionFilterPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
        AU.setPreservesAll();
}
char FunctionFilterPass::ID = 0;
std::string demangle(const std::string &mangled_name) {
        int status = -1; 
        char *demangled =
                abi::__cxa_demangle(mangled_name.c_str(), NULL, NULL, &status);
        if (status == 0) {
                return std::string(demangled);
        }   
        return std::string();
}

void extract_function_name(std::string &full_name) {
  auto name_end = full_name.find_first_of("(");
  if (name_end != std::string::npos) {
    full_name = full_name.substr(0, name_end);
  }
}
void FunctionFilterPass::loadFile(llvm::Module &M, std::string file_name){
        std::ifstream functions_strm(file_name);
        if (!functions_strm.is_open()) {
                llvm::dbgs() << " failed to open file\n";
                return;
        }   
	llvm::dbgs()<<"Filter file:"<<file_name<<"\n";
        std::string name;
        std::unordered_set<std::string> function_names;
        while (!functions_strm.eof()) {
                functions_strm >> name;
                llvm::dbgs() << "here!:" << name << "\n";
                function_names.insert(name);
        }   
        llvm::dbgs() << "got filter function names\n";
        for (auto &F : M) {
                auto demangled_name = demangle(F.getName());
                if (demangled_name.empty()) {
                    demangled_name = F.getName();
                }   
                //extract_function_name(demangled_name);
                if (function_names.find(demangled_name) !=
                                function_names.end() || function_names.find(F.getName()) != function_names.end()) {
                        llvm::dbgs() << "Add filter function " << demangled_name << "\n";
                        this->m_functions_info.add_function(&F);
                } else {
                    llvm::dbgs() << "did not find " << F.getName() << " demangled to " << demangled_name << "\n";
                }
        }   
        if(this->m_functions_info.get_functions().size()!=function_names.size()) {
            errs()<<"ERR. filter functions count:" << function_names.size()
                  << "!=" << "detected functions in binary is:" << this->m_functions_info.get_functions().size() << "\n";
            dbgs()<<"Detected functions:\n";
            for(auto &F: m_functions_info.get_functions()){
                dbgs()<<F->getName()<<"\t";
            }	
            dbgs()<<"\n";
            dbgs()<<"Filter functions:\n";
            for(std::string S: function_names){
                dbgs()<<S<<"\t";
            }
            dbgs()<<"\n";
            exit(1);
        }

}
bool FunctionFilterPass::runOnModule(llvm::Module &M) {
        llvm::dbgs() << "In  filter function pass "<< "\n";
        this->m_functions_info.init();
	if(!FilterFile.empty())
		loadFile(M,FilterFile);
        return false;
}
FunctionInformation* FunctionFilterPass::get_functions_info() {
        return &(this->m_functions_info);
}
static llvm::RegisterPass<FunctionFilterPass>
X("filter-func", "Include functions in a given file in any transformation");
