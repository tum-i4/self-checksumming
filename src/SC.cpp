#include "CheckersNetwork.h"
#include "input-dependency/InputDependencyAnalysisPass.h"
#include "input-dependency/FunctionInputDependencyResultInterface.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <limits.h>
#include <stdint.h>
#include "FunctionMarker.h"
#include "FunctionFilter.h"
#include "Stats.h"

using namespace llvm;

static cl::opt<bool> InputDependentFunctionsOnly(
    "input-dependent-functions", cl::Hidden,
    cl::desc("Only input dependent functions are protected using SC "));

static cl::opt<int> DesiredConnectivity(
    "connectivity", cl::Hidden,
    cl::desc("The desired level of connectivity of checkers node in the network "));

static cl::opt<std::string> LoadCheckersNetwork(
    "load-checkers-network", cl::Hidden,
    cl::desc("File path to load checkers' network in Json format "));

static cl::opt<std::string> DumpSCStat(
    "dump-sc-stat", cl::Hidden,
    cl::desc("File path to dump pass stat in Json format "));


static cl::opt<std::string> DumpCheckersNetwork(
    "dump-checkers-network", cl::Hidden,
    cl::desc("File path to dump checkers' network in Json format "));

namespace {

struct SCPass : public ModulePass {
  Stats stats;
  static char ID;
  SCPass() : ModulePass(ID) {}


  /*long getFuncInstructionCount(const Function &F){
      long count=0;
      for (BasicBlock& bb : F){
	count += std::distance(bb.begin(), bb.end());
      }
      return count;
  }*/
  virtual bool runOnModule(Module &M) {
      bool didModify = false;
      std::vector<Function *> allFunctions;
      const auto &input_dependency_info =
          getAnalysis<input_dependency::InputDependencyAnalysisPass>().getInputDependencyAnalysis();
      //const auto &assert_function_info =
      //getAnalysis<AssertFunctionMarkPass>().get_assert_functions_info();
      auto* function_info =
          getAnalysis<FunctionMarkerPass>().get_functions_info();
      auto function_filter_info =
          getAnalysis<FunctionFilterPass>().get_functions_info();

      int countProcessedFuncs=0;
      for (auto &F : M) {
          if (F.isDeclaration() || F.size() == 0 || F.getName()=="guardMe")
              continue;
          if (function_filter_info->get_functions().size() !=0 && 
                  !function_filter_info->is_function(&F)){
              llvm::dbgs() << "SC skipped function:" << F.getName()
                           << " because it is not in the FunctionFilterPass list.\n";
              continue;
          }
          countProcessedFuncs++;
          auto F_input_dependency_info = input_dependency_info->getAnalysisInfo(&F);
          if (!F_input_dependency_info) {
              dbgs() << "Skipping function because it has no input dependency result " << F.getName() << "\n";
              continue;
          }
          // no checksum for deterministic functions
          // only when input-dependent-functions flag is set
          if (InputDependentFunctionsOnly && !F_input_dependency_info->isInputDepFunction()) {
              dbgs() << "Skipping function because it is input independent "
                     << F.getName() << "\n";
              continue;
          } else if (function_info->get_functions().size() !=0 &&
                  !function_info->is_function(&F)) {
              llvm::dbgs() << "SC skipped function:" << F.getName()
                           << " because it is not in the SC include list!\n";
              continue;
          } else {
              llvm::dbgs() << "SC included function:" << F.getName()
                           << " because it is in the SC include list/ or no list is provided!\n";
          }
          // Collect all functions in module
          allFunctions.push_back(&F);
      }
      int totalNodes = allFunctions.size();
      // TODO: recieve desired connectivity from commandline
      if( DesiredConnectivity == 0) {
          DesiredConnectivity=2;
      }
      dbgs()<<"DesiredConnectivity is :"<<DesiredConnectivity<<"\n";
      CheckersNetwork checkerNetwork;

      // map functions to checker checkee map nodes
      std::list<Function*> topologicalSortFuncs;
      std::map<Function*, std::vector<Function*>> checkerFuncMap;
      std::vector<int> actucalConnectivity;
      if(!LoadCheckersNetwork.empty()) {
          checkerFuncMap= checkerNetwork.loadJson(LoadCheckersNetwork, M, topologicalSortFuncs);
          if (!DumpSCStat.empty()){
              //TODO: maybe we dump the stats into the JSON file and reload it just like the network
              errs()<<"ERR. Stats is not avalilable for the loaded networks...";
          }
      } else {
          checkerNetwork.constructAcyclicCheckers(totalNodes, DesiredConnectivity, actucalConnectivity);
          dbgs() << "Constructed the network of checkers!\n";
          checkerFuncMap = checkerNetwork.mapCheckersOnFunctions(allFunctions,
                                                                 topologicalSortFuncs,
                                                                 M);
      }
      if (!DumpCheckersNetwork.empty()) {
          dbgs() << "Dumping checkers network info\n";
          checkerNetwork.dumpJson(checkerFuncMap, DumpCheckersNetwork, topologicalSortFuncs);
      } else {
          dbgs() << "No checkers network info file is requested!\n";
      }
      unsigned int marked_function_count = 0;

      //Stats function list
      std::map<Function *, int> ProtectedFuncs;
      int numberOfGuards = 0;
      int numberOfGuardInstructions = 0;

      // inject one guard for each item in the checkee vector
      for (auto &F : topologicalSortFuncs) {
          auto it = checkerFuncMap.find(F);
          if (it == checkerFuncMap.end())
              continue;
          auto &BB = F->getEntryBlock();
          auto I = BB.getFirstNonPHIOrDbg();

          auto F_input_dependency_info = input_dependency_info->getAnalysisInfo(F);
          for (auto &Checkee : it->second) {
              //This is all for the sake of the stats
              ++ProtectedFuncs[Checkee];
              //End of stats

              //Note checkees in Function marker pass
              function_info->add_function(Checkee);
              marked_function_count++;
              dbgs() << "Insert guard in " << F->getName()
                     << " checkee: " << Checkee->getName() << "\n";
              numberOfGuards++;
              injectGuard(&BB, I, Checkee,
                         numberOfGuardInstructions,
                         F_input_dependency_info->isInputDepFunction());
              didModify = true;
          }
      }

      //Do we need to dump stats?
      if(!DumpSCStat.empty()){
	  //calc number of sensitive instructions
	  long sensitiveInsts = 0;
	  for(const auto &function:allFunctions){
	    for (BasicBlock& bb : *function){
	      sensitiveInsts += std::distance(bb.begin(), bb.end());
	    }
	  }
	  stats.setNumberOfSensitiveInstructions(sensitiveInsts);
          stats.addNumberOfGuards(numberOfGuards);
          stats.addNumberOfProtectedFunctions(ProtectedFuncs.size());
          stats.addNumberOfGuardInstructions(numberOfGuardInstructions);
          stats.setDesiredConnectivity(DesiredConnectivity);
          long protectedInsts = 0;
          std::vector<int> frequency;
          for (const auto &item:ProtectedFuncs){
              const auto &function = item.first;
              const int frequencyOfChecks = item.second;
              for (BasicBlock& bb : *function){
	         protectedInsts += std::distance(bb.begin(), bb.end());
	      }
              frequency.push_back(frequencyOfChecks);
          }
          stats.addNumberOfProtectedInstructions(protectedInsts);
          stats.calculateConnectivity(frequency);
          dbgs()<<"SC stats is requested, dumping stat file...\n";
          stats.dumpJson(DumpSCStat);
      }


      const auto &funinfo = getAnalysis<FunctionMarkerPass>().get_functions_info();
      llvm::dbgs() << "Recieved marked functions "<<funinfo->get_functions().size()<<"\n";
      if(marked_function_count!=funinfo->get_functions().size()) {
          llvm::dbgs() << "ERR. Marked functions "<<marked_function_count<<" are not reflected correctly "<<funinfo->get_functions().size()<<"\n";
      }
      //Make sure OH only processed filter function list
      if(countProcessedFuncs!=function_filter_info->get_functions().size() 
              && function_filter_info->get_functions().size()>0){
          errs()<<"ERR. processed "<<countProcessedFuncs<<" function, while filter count is "<<function_filter_info->get_functions().size()<<"\n";
          exit(1);
      }

      return didModify;
  }

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
    AU.addRequired<input_dependency::InputDependencyAnalysisPass>();
    AU.addRequired<FunctionMarkerPass>();
    AU.addPreserved<FunctionMarkerPass>();
    AU.addRequired<FunctionFilterPass>();
    AU.addPreserved<FunctionFilterPass>();
  }
  uint64_t rand_uint64(void) {
    uint64_t r = 0;
    for (int i = 0; i < 64; i += 30) {
      r = r * ((uint64_t)RAND_MAX + 1) + rand();
    }
    return r;
  }
  void appendToPatchGuide(const short length, const int address,
                          const int expectedHash, std::string functionName) {
    FILE *pFile;
    pFile = fopen("guide.txt", "a");
    fprintf(pFile, "%s,%d,%d,%d\n", functionName.c_str(), address, length,
            expectedHash);
    fclose(pFile);
  }
  void setPatchMetadata(Instruction *Inst, std::string tag) {
    LLVMContext &C = Inst->getContext();
    MDNode *N = MDNode::get(C, MDString::get(C, tag));
    Inst->setMetadata("guard", N);
  }
  void injectGuard(BasicBlock *BB, Instruction *I, Function *Checkee,
                   int &numberOfGuardInstructions,
                   bool is_in_inputdep) {
    LLVMContext &Ctx = BB->getParent()->getContext();
    // get BB parent -> Function -> get parent -> Module
    Constant *guardFunc = BB->getParent()->getParent()->getOrInsertFunction(
        "guardMe", Type::getVoidTy(Ctx), Type::getInt32Ty(Ctx),
        Type::getInt16Ty(Ctx), Type::getInt32Ty(Ctx), NULL);

    IRBuilder<> builder(I);
    auto insertPoint = ++builder.GetInsertPoint();
    if(llvm::TerminatorInst::classof(I)){
        insertPoint--;
    }
    builder.SetInsertPoint(BB, insertPoint);
    // int8_t address[5] = {0,0,0,0,1};
    short length = rand() % SHRT_MAX; // rand_uint64();;
    int address = rand() % INT_MAX; // rand_uint64();
    int expectedHash = rand() % INT_MAX;
    dbgs() << "placeholder:" << address << " "
           << " size:" << length << " expected hash:" << expectedHash << "\n";
    appendToPatchGuide(length, address, expectedHash, Checkee->getName());
    std::vector<llvm::Value *> args;

    auto* arg1 = llvm::ConstantInt::get(llvm::Type::getInt32Ty(Ctx), address);
    auto* arg2 = llvm::ConstantInt::get(llvm::Type::getInt16Ty(Ctx), length);
    auto* arg3 = llvm::ConstantInt::get(llvm::Type::getInt32Ty(Ctx), expectedHash);
    if (is_in_inputdep) {
        args.push_back(arg1);
        args.push_back(arg2);
        args.push_back(arg3);
    } else {
        auto *A = builder.CreateAlloca(Type::getInt32Ty(Ctx), nullptr, "a");
        auto *B = builder.CreateAlloca(Type::getInt16Ty(Ctx), nullptr, "b");
        auto *C = builder.CreateAlloca(Type::getInt32Ty(Ctx), nullptr, "c");
        auto *store1 = builder.CreateStore(arg1, A, /*isVolatile=*/false);
        // setPatchMetadata(store1, "address");
        auto *store2 = builder.CreateStore(arg2, B, /*isVolatile=*/false);
        // setPatchMetadata(store2, "length");
        auto *store3 = builder.CreateStore(arg3, C, /*isVolatile=*/false);
        // setPatchMetadata(store3, "hash");
        auto *load1 = builder.CreateLoad(A);
        auto *load2 = builder.CreateLoad(B);
        auto *load3 = builder.CreateLoad(C);
        args.push_back(load1);
        args.push_back(load2);
        args.push_back(load3);
    }

    CallInst *call = builder.CreateCall(guardFunc, args);
    setPatchMetadata(call, Checkee->getName());
    Checkee->addFnAttr(llvm::Attribute::NoInline);
    //Stats: we assume the call instrucion and its arguments account for one instruction
    numberOfGuardInstructions+=1;
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
