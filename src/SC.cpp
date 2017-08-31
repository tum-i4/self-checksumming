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
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <limits.h>
#include <stdint.h>
#include "llvm/Support/CommandLine.h"
using namespace llvm;

static cl::opt<bool> InputDependentFunctionsOnly(
        "input-dependent-functions", cl::Hidden,
        cl::desc("Only input dependent functions are protected using SC "));


namespace {
struct SCPass : public ModulePass {
  static char ID;
  SCPass() : ModulePass(ID) {}

  virtual bool runOnModule(Module &M) {
    bool didModify = false;
    std::vector<Function *> allFunctions;
    const auto& input_dependency_info = getAnalysis<input_dependency::InputDependencyAnalysis>();
    const auto& function_calls = getAnalysis<input_dependency::InputDependentFunctionsPass>();
    for (auto &F : M) {
      if (F.isDeclaration() || F.size() == 0)
        continue;

      // no checksum for deterministic functions
      // only when input-dependent-functions flag is set 
      if (InputDependentFunctionsOnly && function_calls.is_function_input_independent(&F)) {
        dbgs()<<"Skipping function because it is input independent "<<F.getName()<<"\n";
        continue;
      }
      // Collect all functions in module
      // TODO: filter list of functions
      allFunctions.push_back(&F);
    }
    int totalNodes = allFunctions.size();
    // TODO: recieve desired connectivity from commandline
    int desiredConnectivity = 2;
    CheckersNetwork checkerNetwork;
    checkerNetwork.constructAcyclicCheckers(totalNodes, desiredConnectivity);

    // map functions to checker checkee map nodes
    std::list<Function *> topologicalSortFuncs;
    std::map<Function *, std::vector<Function *>> checkerFuncMap =
        checkerNetwork.mapCheckersOnFunctions(allFunctions,
                                              topologicalSortFuncs);
    // inject one guard for each item in the checkee vector
    for (auto &F : topologicalSortFuncs) {
      auto it = checkerFuncMap.find(F);
      if (it == checkerFuncMap.end())
        continue;
      auto &BB = F->getEntryBlock();
      auto I = BB.getFirstNonPHIOrDbg();

      for (auto &Checkee : checkerFuncMap[F]) {
        dbgs() << "Insert guard in " << F->getName()
               << " checkee: " << Checkee->getName() << "\n";
        injectGuard(&BB, I, Checkee);
        didModify = true;
      }
    }
    return didModify;
  }

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
    AU.addRequired<input_dependency::InputDependencyAnalysis>();
    AU.addRequired<input_dependency::InputDependentFunctionsPass>();

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
  void injectGuard(BasicBlock *BB, Instruction *I, Function *Checkee) {
    LLVMContext &Ctx = BB->getParent()->getContext();
    // get BB parent -> Function -> get parent -> Module
    Constant *guardFunc = BB->getParent()->getParent()->getOrInsertFunction(
        "guardMe", Type::getVoidTy(Ctx), Type::getInt32Ty(Ctx),
        Type::getInt16Ty(Ctx), Type::getInt32Ty(Ctx), NULL);
    IRBuilder<> builder(I);
    auto insertPoint = ++builder.GetInsertPoint();
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

    // Constant* beginConstAddress = ConstantInt::get(Type::getInt8Ty(Ctx),
    // (int8_t)&address);
    // Value* beginConstPtr = ConstantExpr::getIntToPtr(beginConstAddress ,
    // 	PointerType::getUnqual(Type::getInt8Ty(Ctx)));
    std::vector<llvm::Value *> args;
    args.push_back(arg1);
    args.push_back(arg2);
    args.push_back(arg3);
    builder.SetInsertPoint(BB, insertPoint);
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
