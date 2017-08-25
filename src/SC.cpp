#include "llvm/Pass.h"
#include <stdint.h>
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
			//TODO: Remove hardcoded network of checkers
			if (F.getName()=="f2") return false;
			bool didModify = false;
			for (auto& B : F) {
				for (auto& I : B) {
						injectGuard(&B, &I, F.getName().str());
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
		void appendToPatchGuide(const unsigned short length, 
				const unsigned int address, const unsigned int expectedHash,
				 std::string functionName){
			FILE *pFile;
			//hardcoded acyclic chain of checkers:  main->f1->f2
			//-> denotes check direction
			if (functionName=="main") functionName="f1";
			else if(functionName=="f1") functionName="f2";
			pFile=fopen("guide.txt", "a");
			fprintf(pFile, "%s,%zu,%hu,%zu\n", functionName.c_str(),address,length,expectedHash);
			fclose(pFile);
		}
//		void clearPatchGuide(){
//			 FILE *pFile;
//                       pFile=fopen("guide.txt", "w");
//		}
		void injectGuard(BasicBlock *BB, Instruction *I,std::string funcName){
			LLVMContext& Ctx = BB->getParent()->getContext();
			// get BB parent -> Function -> get parent -> Module
			Constant* guardFunc = BB->getParent()->getParent()->getOrInsertFunction(
					"guardMe", Type::getVoidTy(Ctx), Type::getInt32Ty(Ctx), 
					Type::getInt16Ty(Ctx), Type::getInt32Ty(Ctx), NULL
					);
			IRBuilder <> builder(I);
			auto insertPoint = ++builder.GetInsertPoint();
			// int8_t address[5] = {0,0,0,0,1};
			unsigned short length = rand();//rand_uint64();;

			unsigned int address = rand();//rand_uint64();

			unsigned int expectedHash = rand();

			dbgs()<<"placeholder:"<<address<<" "<<" size:"<<length<<" expected hash:"<<expectedHash<<"\n";

			appendToPatchGuide(length,address,expectedHash,funcName);
			Value *arg1 = builder.getInt32(address);
			Value *arg2 = builder.getInt16(length);
			Value *arg3 = builder.getInt32(expectedHash);

			// Constant* beginConstAddress = ConstantInt::get(Type::getInt8Ty(Ctx), (int8_t)&address);
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
