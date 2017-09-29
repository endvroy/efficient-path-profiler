

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/GraphWriter.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "PathProfilingPass.h"
#include <vector>

using namespace llvm;
using namespace pathprofiling;


namespace pathprofiling {
    char PathProfilingPass::ID = 0;
}


bool
PathProfilingPass::runOnModule(llvm::Module &module) {
    // Implement your solution here.
    auto &ep = getAnalysis<PathEncodingPass>();
    for (auto fn:ep.toInstr) {
        instrument(module, *fn);
    }
    return true;
}

BasicBlock *safeSplitEdge(BasicBlock *pred, BasicBlock *succ) {
    auto result = SplitCriticalEdge(pred, succ);
    if (result) {
        return result;
    }
    else {
        if (pred->getUniqueSuccessor()) {
            return pred;
        }
        else {
            return succ;
        }
    }
}


void
PathProfilingPass::instrument(llvm::Module &module,
                              llvm::Function &function) {
    // You may want to implement this function as a part of your solution.
    auto &entrybb = function.getEntryBlock();
    auto &i = *entrybb.begin();
    IRBuilder<> entryBuilder(&i);
    auto acc = entryBuilder.CreateAlloca(entryBuilder.getInt64Ty());
    entryBuilder.CreateStore(entryBuilder.getInt64(0), acc);

//    outs() << "I survived -1\n";

    auto &ep = getAnalysis<PathEncodingPass>();
//    ep.debugPrint();
    auto allEdges = ep.fnEdgeMap[&function];
    for (auto p:allEdges) {
        auto &edge = p.first;
        auto label = p.second;
        auto pred = edge.first;
        auto succ = edge.second;
        auto bb = safeSplitEdge(pred, succ);

//        outs() << "I survived -2\n";

        IRBuilder<> builder(bb->getTerminator());
        auto delta = builder.getInt64(label);
        auto accVal = builder.CreateLoad(acc);
        auto result = builder.CreateAdd(accVal, delta);
        builder.CreateStore(result, acc);
    }

}

