#include "self-checksumming/DAGCheckersNetwork.h"
#include "self-checksumming/FunctionFilter.h"
#include "self-checksumming/FunctionMarker.h"
#include "self-checksumming/Stats.h"
#include "input-dependency/Analysis/FunctionInputDependencyResultInterface.h"
#include "input-dependency/Analysis/InputDependencyAnalysisPass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <limits.h>
#include <stdint.h>
#include <cxxabi.h>

using namespace llvm;
static cl::opt<bool> UseOtherFunctions(
    "use-other-functions", cl::Hidden,
    cl::desc("This allows SC to use other functions, beyond the specified filter set, as checkers to meet the desired connectivity level"));

static cl::opt<bool> SensitiveOnlyChecked(
    "sensitive-only-checked", cl::Hidden,
    cl::desc("Sensitive functions are only checked and never used as checkers. Extracted only always is given higher priority, that is sensitive functions are never checkers when extracted only is set"));


static cl::opt<bool> ExtractedOnly(
    "extracted-only", cl::Hidden,
    cl::desc("Only extracted functions are protected using SC, extracted functions in are always checkees and never checkers, this mode uses other functions regardless of setting use-other-functions flag or not. "));

static cl::opt<int> DesiredConnectivity(
    "connectivity", cl::Hidden,
    cl::desc(
        "The desired level of connectivity of checkers node in the network "));

static cl::opt<int> MaximumPercOtherFunctions (
    "maximum-other-percentage", cl::Hidden,
    cl::desc("The maximum usage percentage (between 0 and 100) of other functions (beyond the filter set) that should be also "
             "included in the SC protection "));

static cl::opt<std::string> LoadCheckersNetwork(
    "load-checkers-network", cl::Hidden,
    cl::desc("File path to load checkers' network in Json format "));

static cl::opt<std::string>
    DumpSCStat("dump-sc-stat", cl::Hidden,
               cl::desc("File path to dump pass stat in Json format "));

static cl::opt<std::string> DumpCheckersNetwork(
    "dump-checkers-network", cl::Hidden,
    cl::desc("File path to dump checkers' network in Json format "));

namespace {

std::string demangle_name(const std::string& name)
{
    int status = -1;
    char* demangled = abi::__cxa_demangle(name.c_str(), nullptr, nullptr, &status);
    if (status != 0) {
        return name;
    }
    std::string demangled_name(demangled);
    demangled_name.erase(std::remove(demangled_name.begin(), demangled_name.end(), ' '), demangled_name.end());
    for (char &c : demangled_name) {
      if (c == '(' || c == '*' || c == '&'
            || c == ')' || c == ',' || c == '<' || c == '>'
            || c == '~' || c == '[' || c == ']') {
            c = '_';
        }
    }
    return demangled_name;
}


struct SCPass : public ModulePass {
  Stats stats;
  static char ID;
  SCPass() : ModulePass(ID) {}

  llvm::MDNode* sc_guard_md{};
  const std::string sc_guard_str = "sc_guard";

  /*long getFuncInstructionCount(const Function &F){
      long count=0;
      for (BasicBlock& bb : F){
        count += std::distance(bb.begin(), bb.end());
      }
      return count;
  }*/


  bool assert_sensitive_only_checked_condition(const std::vector<Function *> sensitiveFunctions,
		                               const std::map<Function *, std::vector<Function *>> &checkerFuncMap){
    for(auto &func: sensitiveFunctions){
      if(checkerFuncMap.find(func)!=checkerFuncMap.end()){
	errs() << "Sensitive functions are checkers while SensitiveOnlyChecked is set to:"<<SensitiveOnlyChecked<<"\n";
	exit(1);
      }
    }
    dbgs() << "Sensitive functions are never checkers as SensitiveOnlyChecked is set to:"<<SensitiveOnlyChecked<<"\n";
  }

  bool runOnModule(Module &M) override {
    bool didModify = false;
    std::vector<Function *> sensitiveFunctions,
        otherFunctions;
    const auto &input_dependency_info =
        getAnalysis<input_dependency::InputDependencyAnalysisPass>()
            .getInputDependencyAnalysis();
    auto *function_info =
        getAnalysis<FunctionMarkerPass>().get_functions_info();
    auto function_filter_info =
        getAnalysis<FunctionFilterPass>().get_functions_info();

    auto* sc_guard_md_str = llvm::MDString::get(M.getContext(), sc_guard_str);
    sc_guard_md = llvm::MDNode::get(M.getContext(), sc_guard_md_str);

    int countProcessedFuncs = 0;
    for (auto &F : M) {
      if (F.isDeclaration() || F.empty() || F.getName() == "guardMe")
        continue;

      countProcessedFuncs++;
      auto F_input_dependency_info = input_dependency_info->getAnalysisInfo(&F);

      // TODO: Why skipping such functions?
      if (!F_input_dependency_info) {
        dbgs() << "Skipping function because it has no input dependency result "
               << F.getName() << "\n";
        continue;
      }
      bool isExtracted =  F_input_dependency_info->isExtractedFunction();
      bool isSensitive = ExtractedOnly? isExtracted: true; //only extracted functions if ExtarctedOnly is set 
      //honor the filter function list
      if (!function_filter_info->get_functions().empty() &&
          !function_filter_info->is_function(&F)) {
        isSensitive = false;
      }
      //ExtractedOnly flag enforces the usage of other functions, regardless of the UseOtherFunctions flag
      if (ExtractedOnly && (!isExtracted)) {
        dbgs() << "Adding " << F.getName() << " other functions, ExtractedOnly mode uses other functions\n";
        otherFunctions.push_back(&F);
      } else if (!ExtractedOnly && UseOtherFunctions && !isSensitive){
        dbgs() << "Adding " << F.getName() << " other functions, UseOtherFunctions mode\n";
	otherFunctions.push_back(&F);
      }
      else if (isSensitive) {
        dbgs() << "Adding " << F.getName() << " to sensitive vector\n";
        sensitiveFunctions.push_back(&F);
      } 
    }

    auto rng = std::default_random_engine{};

    dbgs() << "Sensitive functions:" << sensitiveFunctions.size()
           << " other functions:" << otherFunctions.size() << "\n";
    // shuffle all functions
    std::shuffle(std::begin(sensitiveFunctions), std::end(sensitiveFunctions),
                 rng);
    std::shuffle(std::begin(otherFunctions), std::end(otherFunctions), rng);

    if (DesiredConnectivity == 0) {
      DesiredConnectivity = 2;
    }
    dbgs() << "DesiredConnectivity is :" << DesiredConnectivity << "\n";

    // Implement #43
    /*int totalNodes = sensitiveFunctions.size() + DesiredConnectivity;
    int actual_connectivity = DesiredConnectivity;
    bool accept_lower_connectivity = false;
    //make sure we can satisfy this requirement, i.e. we have enough functions
    if (DesiredConnectivity > otherFunctions.size()){
	//adjust actual connectivity
	dbgs()<<"SCPass. There is not enough functions in the module to satisfy the desired connectivity...\n";
	//TODO: decide whether carrying on or downgrading connectivity is better 
	//actual_connectivity = otherFunctions.size()+availableInputIndependents;     
	//dbgs()<<"Actual connectivity is:"<<actual_connectivity<<"\n";
	dbgs()<<"Carrying on with the desired connectivity nonetheless";
	accept_lower_connectivity = true;
    }*/


    //dbgs() << "Total nodes:" << totalNodes << "\n";
    //int availableOtherFunction = 0;
      // indicates that we need to take some other functions
    //availableOtherFunction = actual_connectivity;
    //dbgs() << "available other functions:" << availableOtherFunction << "\n";


       //if (availableOtherFunction > 0) {
      //for (Function *func : otherFunctions) {
       // dbgs() << "pushing back other input dependent function "
        //       << func->getName() << "\n";
       // allFunctions.push_back(func);
       // availableOtherFunction--;
       // if (availableOtherFunction <= 0)
        //  break;
     // }
   // }

    dbgs() << "Other functions to be fed to the network of checkers\n";
    for (auto &F : otherFunctions) {
      dbgs() << F->getName() << "\n";
    }
    dbgs() << "***\n";
    dbgs() << "Sensitive functions to be fed to the network of checkers\n";
    for (auto &F : sensitiveFunctions) {
      dbgs() << F->getName() << "\n";
    }
    dbgs() << "***\n";
    dbgs()<<"Sensitive functions only checked:"<<SensitiveOnlyChecked<<"\n";

    DAGCheckersNetwork checkerNetwork;
    checkerNetwork.setLowerConnectivityAcceptance(true);
    // map functions to checker checkee map nodes
    std::list<Function *> topologicalSortFuncs;
    std::map<Function *, std::vector<Function *>> checkerFuncMap;
    std::vector<int> actucalConnectivity;
    if (!LoadCheckersNetwork.empty()) {
      checkerFuncMap =
          checkerNetwork.loadJson(LoadCheckersNetwork.getValue(), M, topologicalSortFuncs);
      if (!DumpSCStat.empty()) {
        // TODO: maybe we dump the stats into the JSON file and reload it just
        // like the network
        errs() << "ERR. Stats is not avalilable for the loaded networks...";
	exit(1);
      }
    } else {
      if(!SensitiveOnlyChecked && !ExtractedOnly)//SensitiveOnlyChecked prevents sensitive function being picked as checkers, extracted functions are never checkers 
      {
        otherFunctions.insert(otherFunctions.end(), sensitiveFunctions.begin(),
                              sensitiveFunctions.end());
      }
      checkerFuncMap = checkerNetwork.constructProtectionNetwork(
          sensitiveFunctions, otherFunctions, DesiredConnectivity);
      topologicalSortFuncs = checkerNetwork.getReverseTopologicalSort(checkerFuncMap);
      dbgs() << "Constructed the network of checkers!\n";
      if(SensitiveOnlyChecked || ExtractedOnly){
	 assert_sensitive_only_checked_condition(sensitiveFunctions,checkerFuncMap);
      }
    }
    if (!DumpCheckersNetwork.empty()) {
      dbgs() << "Dumping checkers network info\n";
      checkerNetwork.dumpJson(checkerFuncMap, DumpCheckersNetwork.getValue(),
                              topologicalSortFuncs);
    } else {
      dbgs() << "No checkers network info file is requested!\n";
    }
    unsigned int marked_function_count = 0;

    // Stats function list
    std::map<Function *, int> ProtectedFuncs;
    int numberOfGuards = 0;
    int numberOfGuardInstructions = 0;

    // inject one guard for each item in the checkee vector
    // reverse topologically sorted 
    for (auto &F : topologicalSortFuncs) {
      auto it = checkerFuncMap.find(F);
      if (it == checkerFuncMap.end())
        continue;
      auto &BB = F->getEntryBlock();
      auto I = BB.getFirstNonPHIOrDbg();

      auto F_input_dependency_info = input_dependency_info->getAnalysisInfo(F);
      for (auto &Checkee : it->second) {
        // This is all for the sake of the stats
	//only collect connectivity info for sensitive functions
	if(std::find(sensitiveFunctions.begin(), sensitiveFunctions.end(),Checkee)!=sensitiveFunctions.end()) 
          ++ProtectedFuncs[Checkee];
        // End of stats

        // Note checkees in Function marker pass
        function_info->add_function(Checkee);
        marked_function_count++;
        dbgs() << "Insert guard in " << F->getName()
               << " checkee: " << Checkee->getName() << "\n";
        numberOfGuards++;
        injectGuard(&BB, I, Checkee, numberOfGuardInstructions,
		   false);// F_input_dependency_info->isInputDepFunction() || F_input_dependency_info->isExtractedFunction());
        didModify = true;
      }
    }

    // Do we need to dump stats?
    if (!DumpSCStat.empty()) {
      // calc number of sensitive instructions
      long sensitiveInsts = 0;
      for (const auto &function : sensitiveFunctions) {
        for (BasicBlock &bb : *function) {
          sensitiveInsts += std::distance(bb.begin(), bb.end());
        }
      }
      stats.setNumberOfSensitiveInstructions(sensitiveInsts);
      stats.addNumberOfGuards(numberOfGuards);
      stats.addNumberOfProtectedFunctions(static_cast<int>(ProtectedFuncs.size()));
      stats.addNumberOfGuardInstructions(numberOfGuardInstructions);
      stats.setDesiredConnectivity(DesiredConnectivity);
      long protectedInsts = 0;
      std::vector<int> frequency;
      for (const auto &item : ProtectedFuncs) {
        const auto &function = item.first;
        const int frequencyOfChecks = item.second;
        for (BasicBlock &bb : *function) {
          protectedInsts += std::distance(bb.begin(), bb.end());
        }
        frequency.push_back(frequencyOfChecks);
      }
      stats.addNumberOfProtectedInstructions(protectedInsts);
      stats.calculateConnectivity(frequency);
      //stats.setAvgConnectivity(actual_connectivity);
      //stats.setStdConnectivity(0);
      dbgs() << "SC stats is requested, dumping stat file...\n";
      stats.dumpJson(DumpSCStat.getValue());
    }

    const auto &funinfo =
        getAnalysis<FunctionMarkerPass>().get_functions_info();
    llvm::dbgs() << "Recieved marked functions "
                 << funinfo->get_functions().size() << "\n";
    if (marked_function_count != funinfo->get_functions().size()) {
      llvm::dbgs() << "ERR. Marked functions " << marked_function_count
                   << " are not reflected correctly "
                   << funinfo->get_functions().size() << "\n";
    }
    // Make sure OH only processed filter function list
    if (countProcessedFuncs != function_filter_info->get_functions().size() &&
        !function_filter_info->get_functions().empty()) {
      errs() << "ERR. processed " << countProcessedFuncs
             << " function, while filter count is "
             << function_filter_info->get_functions().size() << "\n";
      //exit(1);
    }

    return didModify;
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesAll();
    AU.addRequired<input_dependency::InputDependencyAnalysisPass>();
    AU.addRequired<FunctionMarkerPass>();
    AU.addPreserved<FunctionMarkerPass>();
    AU.addRequired<FunctionFilterPass>();
    AU.addPreserved<FunctionFilterPass>();
  }
  uint64_t rand_uint64() {
    uint64_t r = 0;
    for (int i = 0; i < 64; i += 30) {
      r = r * ((uint64_t)RAND_MAX + 1) + rand();
    }
    return r;
  }
  void appendToPatchGuide(const unsigned int length, const unsigned int address,
                          const unsigned int expectedHash, const std::string &functionName) {
    FILE *pFile;
    pFile = fopen("guide.txt", "a");
    std::string demangled_name = demangle_name(functionName);
    fprintf(pFile, "%s,%d,%d,%d\n", demangled_name.c_str(), address, length,
            expectedHash);
    fclose(pFile);
  }

  void setPatchMetadata(Instruction *Inst, const std::string &tag) {
    LLVMContext &C = Inst->getContext();
    MDNode *N = MDNode::get(C, MDString::get(C, tag));
    Inst->setMetadata("guard", N);
  }

  unsigned int size_begin = 555555555;
  unsigned int address_begin = 222222222;
  unsigned int expected_hash_begin = 444444444; 
  void injectGuard(BasicBlock *BB, Instruction *I, Function *Checkee,
                   int &numberOfGuardInstructions, bool is_in_inputdep) {
    LLVMContext &Ctx = BB->getParent()->getContext();
    // get BB parent -> Function -> get parent -> Module
    llvm::ArrayRef<llvm::Type *> params;
    params = {Type::getInt32Ty(Ctx), Type::getInt32Ty(Ctx), Type::getInt32Ty(Ctx)};
    llvm::FunctionType *function_type = llvm::FunctionType::get(llvm::Type::getVoidTy(Ctx), params, false);
    Constant *guardFunc = BB->getParent()->getParent()->getOrInsertFunction("guardMe", function_type);

    IRBuilder<> builder(I);
    auto insertPoint = ++builder.GetInsertPoint();
    if (llvm::TerminatorInst::classof(I)) {
      insertPoint--;
    }
    builder.SetInsertPoint(BB, insertPoint);
    unsigned int length = size_begin++;
    unsigned int address = address_begin++;
    unsigned int expectedHash = expected_hash_begin++;


    dbgs() << "placeholder:" << address << " "
           << " size:" << length << " expected hash:" << expectedHash << "\n";
    appendToPatchGuide(length, address, expectedHash, Checkee->getName());
    std::vector<llvm::Value *> args;

    auto *arg1 = llvm::ConstantInt::get(llvm::Type::getInt32Ty(Ctx), address);
    auto *arg2 = llvm::ConstantInt::get(llvm::Type::getInt32Ty(Ctx), length);
    auto *arg3 =
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(Ctx), expectedHash);
    if (is_in_inputdep) {
      args.push_back(arg1);
      args.push_back(arg2);
      args.push_back(arg3);
      numberOfGuardInstructions += 1;
    } else {
      auto *A = builder.CreateAlloca(Type::getInt32Ty(Ctx), nullptr, "a");
      auto *B = builder.CreateAlloca(Type::getInt32Ty(Ctx), nullptr, "b");
      auto *C = builder.CreateAlloca(Type::getInt32Ty(Ctx), nullptr, "c");
      auto *store1 = builder.CreateStore(arg1, A, /*isVolatile=*/false);
      store1->setMetadata(sc_guard_str, sc_guard_md);
      // setPatchMetadata(store1, "address");
      auto *store2 = builder.CreateStore(arg2, B, /*isVolatile=*/false);
      store2->setMetadata(sc_guard_str, sc_guard_md);
      // setPatchMetadata(store2, "length");
      auto *store3 = builder.CreateStore(arg3, C, /*isVolatile=*/false);
      store3->setMetadata(sc_guard_str, sc_guard_md);
      // setPatchMetadata(store3, "hash");
      auto *load1 = builder.CreateLoad(A);
      load1->setMetadata(sc_guard_str, sc_guard_md);
      auto *load2 = builder.CreateLoad(B);
      load2->setMetadata(sc_guard_str, sc_guard_md);
      auto *load3 = builder.CreateLoad(C);
      load3->setMetadata(sc_guard_str, sc_guard_md);
      args.push_back(load1);
      args.push_back(load2);
      args.push_back(load3);

      numberOfGuardInstructions += 9;
    }

    CallInst *call = builder.CreateCall(guardFunc, args);
    call->setMetadata(sc_guard_str, sc_guard_md);
    setPatchMetadata(call, Checkee->getName());
    Checkee->addFnAttr(llvm::Attribute::NoInline);
    // Stats: we assume the call instrucion and its arguments account for one
    // instruction
  }
};
}

char SCPass::ID = 0;
static llvm::RegisterPass<SCPass> X("sc", "Instruments bitcode with guards");
// Automatically enable the pass.
// http://adriansampson.net/blog/clangpass.html
static void registerSCPass(const PassManagerBuilder &,
                           legacy::PassManagerBase &PM) {
  PM.add(new SCPass());
}
static RegisterStandardPasses
    RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible, registerSCPass);
