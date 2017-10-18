#include "CheckersNetwork.h"
#include "input-dependency/InputDependencyAnalysis.h"
#include "input-dependency/InputDependentFunctions.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <limits.h>
#include <stdint.h>
#include "FunctionMarker.h"
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


static cl::opt<std::string> DumpCheckersNetwork(
    "dump-checkers-network", cl::Hidden,
    cl::desc("File path to dump checkers' network in Json format "));

namespace {
struct SCPass : public ModulePass {
  static char ID;
  SCPass() : ModulePass(ID) {}

  virtual bool runOnModule(Module &M) {
    bool didModify = false;
    std::vector<Function *> allFunctions;
    const auto &input_dependency_info =
        getAnalysis<input_dependency::InputDependencyAnalysis>();
    const auto &function_calls =
        getAnalysis<input_dependency::InputDependentFunctionsPass>();
    //const auto &assert_function_info =
      //getAnalysis<AssertFunctionMarkPass>().get_assert_functions_info();
    dbgs()<<"got here!";
 auto function_info =
      getAnalysis<FunctionMarkerPass>().get_functions_info();
    
    for (auto &F : M) {
      if (F.isDeclaration() || F.size() == 0 || F.getName()=="guardMe")
        continue;

      // no checksum for deterministic functions
      // only when input-dependent-functions flag is set
      if (InputDependentFunctionsOnly &&
          function_calls.is_function_input_independent(&F)) {
        dbgs() << "Skipping function because it is input independent "
               << F.getName() << "\n";
        continue;
      } else  if (function_info->get_functions().size() !=0 &&
           !function_info->is_function(&F)) {
         llvm::dbgs() << "SC skipped function:" << F.getName()
                       << " because it is not in the SC include list!\n";
        continue;
      } else {
         llvm::dbgs() << "SC included function:" << F.getName()
                        << " because it is in the SC include list/ or no list is provided!\n";
      }
      // Collect all functions in module
      // TODO: filter list of functions
      allFunctions.push_back(&F);
    }
    int totalNodes = allFunctions.size();
    // TODO: recieve desired connectivity from commandline
    if( DesiredConnectivity == 0) {DesiredConnectivity=2;}
    dbgs()<<"DesiredConnectivity is :"<<DesiredConnectivity<<"\n";
    CheckersNetwork checkerNetwork;
    // map functions to checker checkee map nodes
    std::list<Function *> topologicalSortFuncs;
    std::map<Function *, std::vector<Function *>> checkerFuncMap;
  if(!LoadCheckersNetwork.empty()){
	  checkerFuncMap= checkerNetwork.loadJson(LoadCheckersNetwork,M,topologicalSortFuncs);
  }else{

    checkerNetwork.constructAcyclicCheckers(totalNodes, DesiredConnectivity);
   dbgs()<<"Constructed the network of checkers!\n";
   checkerFuncMap =
        checkerNetwork.mapCheckersOnFunctions(allFunctions,
                                              topologicalSortFuncs,M);
  }
    if (!DumpCheckersNetwork.empty()) {
      dbgs() << "Dumping checkers network info\n";
      checkerNetwork.dumpJson(checkerFuncMap, DumpCheckersNetwork, topologicalSortFuncs);
    } else {
      dbgs() << "No checkers network info file is requested!\n";
    }
    unsigned int marked_function_count = 0;
    // inject one guard for each item in the checkee vector
    for (auto &F : topologicalSortFuncs) {
      auto it = checkerFuncMap.find(F);
      if (it == checkerFuncMap.end())
        continue;
      auto &BB = F->getEntryBlock();
      auto I = BB.getFirstNonPHIOrDbg();

      for (auto &Checkee : checkerFuncMap[F]) {
	//Note checkees in Function marker pass
	function_info->add_function(Checkee);
	marked_function_count++;
        dbgs() << "Insert guard in " << F->getName()
               << " checkee: " << Checkee->getName() << "\n";
        injectGuard(&BB, I, Checkee);
        didModify = true;
      }
    }

const auto &funinfo = getAnalysis<FunctionMarkerPass>().get_functions_info();
  llvm::dbgs() << "Recieved marked functions "<<funinfo->get_functions().size()<<"\n";
  if(marked_function_count!=funinfo->get_functions().size()) {
    llvm::dbgs() << "ERR. Marked functions "<<marked_function_count<<" are not reflected correctly "<<funinfo->get_functions().size()<<"\n";
  }
    return didModify;
  }

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
    AU.addRequired<input_dependency::InputDependencyAnalysis>();
    AU.addRequired<input_dependency::InputDependentFunctionsPass>();
    AU.addRequired<FunctionMarkerPass>();
    AU.addPreserved<FunctionMarkerPass>();
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
  //		void clearPatchGuide(){
  //			 FILE *pFile;
  //                       pFile=fopen("guide.txt", "w");
  //		}

  void setPatchMetadata(Instruction *Inst, std::string tag) {
    LLVMContext &C = Inst->getContext();
    MDNode *N = MDNode::get(C, MDString::get(C, "placeholder"));
    Inst->setMetadata(tag, N);
  }
  void injectGuard(BasicBlock *BB, Instruction *I, Function *Checkee) {
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
    Value *arg1 = builder.getInt32(address);
    Value *arg2 = builder.getInt16(length);
    Value *arg3 = builder.getInt32(expectedHash);

    // auto *A = builder.CreateAlloca(Type::getInt32Ty(Ctx), nullptr, "a");
    // auto *B = builder.CreateAlloca(Type::getInt16Ty(Ctx), nullptr, "b");
    // auto *C = builder.CreateAlloca(Type::getInt32Ty(Ctx), nullptr, "c");

    // auto *store1 = builder.CreateStore(arg1, A, /*isVolatile=*/false);
    // setPatchMetadata(store1, "address");
    // auto *store2 = builder.CreateStore(arg2, B, /*isVolatile=*/false);
    // setPatchMetadata(store2, "length");
    // auto *store3 = builder.CreateStore(arg3, C, /*isVolatile=*/false);
    // setPatchMetadata(store3, "hash");
    // auto *load1 = builder.CreateLoad(arg1);
    // auto *load2 = builder.CreateLoad(arg2);
    // auto *load3 = builder.CreateLoad(arg3);

    // Constant* beginConstAddress = ConstantInt::get(Type::getInt8Ty(Ctx),
    // (int8_t)&address);
    // Value* beginConstPtr = ConstantExpr::getIntToPtr(beginConstAddress ,
    // 	PointerType::getUnqual(Type::getInt8Ty(Ctx)));
    std::vector<llvm::Value *> args;
    args.push_back(arg1);
    args.push_back(arg2);
    args.push_back(arg3);
    builder.CreateCall(guardFunc, args);
  }
  // void printArg(BasicBlock *BB, IRBuilder<> *builder, std::string valueName){
  // 	LLVMContext &context = BB->getParent()->getContext();;
  // 	std::vector<llvm::Type *> args;
  // 	args.push_back(llvm::Type::getInt8PtrTy(context));
  // 	// accepts a char*, is vararg, and returns int
  // 	FunctionType *printfType =
  // 		llvm::FunctionType::get(builder->getInt32Ty(), args, true);
  // 	Constant *printfFunc =
  // 		BB->getParent()->getParent()->getOrInsertFunction("printf",
  // printfType);
  // 	Value *formatStr = builder->CreateGlobalStringPtr("arg = %s\n");
  // 	Value *argument = builder->CreateGlobalStringPtr(valueName);
  // 	std::vector<llvm::Value *> values;
  // 	values.push_back(formatStr);
  // 	values.push_back(argument);
  // 	builder->CreateCall(printfFunc, values);
  // }
  // void printHash(BasicBlock *BB, Instruction *I, bool
  // insertBeforeInstruction){
  // 	LLVMContext& Ctx = BB->getParent()->getContext();
  // 	// get BB parent -> Function -> get parent -> Module
  // 	Constant* logguardFunc =
  // BB->getParent()->getParent()->getOrInsertFunction(
  // 			"logHash", Type::getVoidTy(Ctx),NULL);
  // 	IRBuilder <> builder(I);
  // 	auto insertPoint = ++builder.GetInsertPoint();
  // 	if(insertBeforeInstruction){
  // 		insertPoint--;
  // 		insertPoint--;
  // 	}
  // 	dbgs() << "FuncName: "<<BB->getParent()->getName()<<"\n";
  // 	builder.SetInsertPoint(BB, insertPoint);
  // 	builder.CreateCall(logguardFunc);
  // }
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
