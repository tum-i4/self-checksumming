#include "PatchManifest.h"
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
using namespace llvm;

namespace {
	struct SCPatchPass : public ModulePass {
		static char ID;
		SCPatchPass() : ModulePass(ID) {}

		virtual bool runOnModule(Module &M) {
			bool didModify = false;
			PatchManifest patchManifest;
			//TODO: get manifest name from command line arguments
			patchManifest.readPatchManifest("patch_guide");
			for (auto &F : M) {
				for (auto &B : F) {
					for (auto &I : B) {
						if (auto *callInst = llvm::dyn_cast<llvm::CallInst>(&I)) {
							auto calledF = callInst->getCalledFunction();
							if (calledF && calledF->getName() == "guardMe") {
								patchGuard(callInst, &patchManifest);
								didModify = true;
							}
						}
					}
				}
			}
			return didModify;
		}

		virtual void getAnalysisUsage(AnalysisUsage &AU) const {}
		void patchGuard(llvm::CallInst* guard_call, PatchManifest *patchManifest) {
			llvm::LLVMContext &Ctx = guard_call->getModule()->getContext();
			llvm::IRBuilder<> builder(guard_call);
			builder.SetInsertPoint(guard_call->getParent(), builder.GetInsertPoint());

			llvm::Value* address_val = guard_call->getArgOperand(0);
			llvm::Value* size_val = guard_call->getArgOperand(1);
			llvm::Value* hash_val = guard_call->getArgOperand(2);

			ConstantInt* CI = dyn_cast<ConstantInt>(address_val);
				int address = CI->getSExtValue();
			CI = dyn_cast<ConstantInt>(size_val);
			int size = CI->getSExtValue();
			CI = dyn_cast<ConstantInt>(hash_val);
			int hash = CI->getSExtValue();

			int address_patch = patchManifest->address_patches[address];
			int size_patch = patchManifest->size_patches[size];
			int hash_patch = patchManifest->hash_patches[hash];


			Value *arg1 = builder.getInt32(address_patch);
			Value *arg2 = builder.getInt16(size_patch);
			Value *arg3 = builder.getInt32(hash_patch);

			guard_call->setArgOperand(0, arg1);
			guard_call->setArgOperand(1, arg2);
			guard_call->setArgOperand(2, arg3);
		}
	};
}

char SCPatchPass::ID = 0;
static llvm::RegisterPass<SCPatchPass>
X("scpatch", "Patch guards with expected address, size and hashes");
// Automatically enable the pass.
// http://adriansampson.net/blog/clangpass.html
static void registerSCPatchPass(const PassManagerBuilder &,
		legacy::PassManagerBase &PM) {
	PM.add(new SCPatchPass());
}
static RegisterStandardPasses
RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible, registerSCPatchPass);
