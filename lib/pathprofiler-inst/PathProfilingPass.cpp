

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

    //register rt fn
    auto &context = module.getContext();
    auto *int64Ty = Type::getInt64Ty(context);
    auto *voidTy = Type::getVoidTy(context);
    SmallVector<Type *, 2> arg_types;
    arg_types.push_back(int64Ty);
    arg_types.push_back(int64Ty);
    auto *countTy = FunctionType::get(voidTy, arg_types, false);
    auto *countFn = module.getOrInsertFunction("PaThPrOfIlInG_count", countTy);
    auto *initFn = module.getOrInsertFunction("PaThPrOfIlInG_init", voidTy, nullptr);
    appendToGlobalCtors(module, llvm::cast<Function>(initFn), 0);
    auto *prnFn = module.getOrInsertFunction("PaThPrOfIlInG_print", voidTy, nullptr);
    appendToGlobalDtors(module, llvm::cast<Function>(prnFn), 0);

    auto &ep = getAnalysis<PathEncodingPass>();
    for (auto fn:ep.toInstr) {
        instrument(countFn, *fn);
    }
    return true;
}

bool existEdge(BasicBlock *pred, BasicBlock *succ) {
    for (auto it = succ_begin(pred), et = succ_end(pred); it != et; it++) {
        auto x = *it;
        if (succ == x) {
            return true;
        }
    }
    return false;
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
PathProfilingPass::instrument(llvm::Constant *countFn,
                              llvm::Function &function) {
    // You may want to implement this function as a part of your solution.

    //declare the accumulator
    auto &entrybb = function.getEntryBlock();
    auto &instr = *entrybb.begin();
    IRBuilder<> entryBuilder(&instr);
    auto acc = entryBuilder.CreateAlloca(entryBuilder.getInt64Ty());
    entryBuilder.CreateStore(entryBuilder.getInt64(0), acc);

    //insert add instr
    auto &ep = getAnalysis<PathEncodingPass>();
    auto &allEdges = ep.fnEdgeMap[&function];
    for (auto p:allEdges) {
        auto &edge = p.first;
        auto label = p.second;
        auto pred = edge.first;
        auto succ = edge.second;
        bool check = true;
        while (check && existEdge(pred, succ)) {
            auto bb = safeSplitEdge(pred, succ);
            check = !(bb == pred || bb == succ);

            IRBuilder<> builder(bb->getTerminator());
            auto delta = builder.getInt64(label);
            auto accVal = builder.CreateLoad(acc);
            auto result = builder.CreateAdd(accVal, delta);
            builder.CreateStore(result, acc);
        }
    }

    //save path
    auto &fnId = ep.fnId;
    for (auto &bb:function) {
        auto i = bb.getTerminator();
        auto result = dyn_cast<ReturnInst>(i);
        if (result) {
            IRBuilder<> builder(i);
            auto accVal = builder.CreateLoad(acc);
            SmallVector<Value *, 2> args;
            args.push_back(builder.getInt64(fnId[&function]));
            args.push_back(accVal);
            builder.CreateCall(countFn, args);
        }
    }
}

