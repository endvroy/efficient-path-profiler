
#ifndef PATHENCODINGPASS_H
#define PATHENCODINGPASS_H


#include "llvm/Analysis/LoopInfo.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

#include <vector>

namespace pathprofiling {


    struct PathEncodingPass : public llvm::ModulePass {
        using Edge = std::pair<llvm::BasicBlock *, llvm::BasicBlock *>;
        using EdgeMap = llvm::DenseMap<Edge, uint64_t>;

        static char ID;

        llvm::DenseMap<llvm::BasicBlock *, uint64_t> numPaths;
        llvm::DenseMap<llvm::Function *, EdgeMap> fnEdgeMap;
        std::vector<llvm::Function *> toInstr;
        llvm::DenseMap<llvm::Function *, uint64_t> fnId;

        PathEncodingPass()
                : llvm::ModulePass(ID) {}

        void
        getAnalysisUsage(llvm::AnalysisUsage &au) const override {
            au.addRequired<llvm::LoopInfoWrapperPass>();
            au.setPreservesAll();
        }

        bool runOnModule(llvm::Module &m) override;

        bool encode(llvm::Function &function);

        void calcFnId();

        void debugPrint();
    };


}  // namespace pathprofiling


#endif

