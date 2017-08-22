#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"

using namespace llvm;


namespace {
	struct SCPass : public FunctionPass {
		static char ID;
		SCPass() : FunctionPass(ID) {}
		virtual bool runOnFunction(Function &F){
			bool didModify = false;
			for (auto& B : F) {
				for (auto& I : B) {
						injectGuard(&B, &I);
						didModify = true;
						break;
					//terminator indicates the last block
					// else if(ReturnInst *RI = dyn_cast<ReturnInst>(&I)){
					// 	// Insert *before* ret
					// 	dbgs() << "**returnInst**\n";
					// 	printHash(&B, RI, true);
					// 	didModify = true;
					// }
				}
				break;
			}
			return didModify;
		}

		virtual void getAnalysisUsage(AnalysisUsage &AU) const {
		}
		uint64_t rand_uint64(void) {
		  uint64_t r = 0;
		  for (int i=0; i<64; i += 30) {
		    r = r*((uint64_t)RAND_MAX + 1) + rand();
		  }
		  return r;
		}
		void injectGuard(BasicBlock *BB, Instruction *I){
			LLVMContext& Ctx = BB->getParent()->getContext();
			// get BB parent -> Function -> get parent -> Module
			Constant* guardFunc = BB->getParent()->getParent()->getOrInsertFunction(
					"guardMe", Type::getVoidTy(Ctx), Type::getInt64Ty(Ctx), Type::getInt64Ty(Ctx),NULL
					);
			IRBuilder <> builder(I);
			auto insertPoint = ++builder.GetInsertPoint();
			// int8_t address[5] = {0,0,0,0,1};
			long length = 5;

			long address = rand_uint64();
			Value *arg1 = builder.getInt64(length);
			Value *arg2 = builder.getInt64(address);
			// Constant* beginConstAddress = ConstantInt::get(Type::getInt8Ty(Ctx), (int8_t)&address);
			// Value* beginConstPtr = ConstantExpr::getIntToPtr(beginConstAddress ,
			// 	PointerType::getUnqual(Type::getInt8Ty(Ctx)));
			std::vector<llvm::Value *> args;
			args.push_back(arg1);
			args.push_back(arg2);
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
		// 		BB->getParent()->getParent()->getOrInsertFunction("printf", printfType);
		// 	Value *formatStr = builder->CreateGlobalStringPtr("arg = %s\n");
		// 	Value *argument = builder->CreateGlobalStringPtr(valueName);
		// 	std::vector<llvm::Value *> values;
		// 	values.push_back(formatStr);
		// 	values.push_back(argument);
		// 	builder->CreateCall(printfFunc, values);
		// }
		// void printHash(BasicBlock *BB, Instruction *I, bool insertBeforeInstruction){
		// 	LLVMContext& Ctx = BB->getParent()->getContext();
		// 	// get BB parent -> Function -> get parent -> Module
		// 	Constant* logguardFunc = BB->getParent()->getParent()->getOrInsertFunction(
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
static llvm::RegisterPass<SCPass> X("sc","Instruments bitcode with guards");
// Automatically enable the pass.
// http://adriansampson.net/blog/clangpass.html
static void registerSCPass(const PassManagerBuilder &,
		legacy::PassManagerBase &PM) {
	PM.add(new SCPass());
}
static RegisterStandardPasses
RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible,
		registerSCPass);