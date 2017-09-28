

#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/GraphWriter.h"

#include "PathEncodingPass.h"

#include <algorithm>
#include <vector>
#include <deque>
#include <iostream>

using namespace llvm;
using namespace pathprofiling;


namespace pathprofiling {
    char PathEncodingPass::ID = 0;
}

void PathEncodingPass::debugPrint() {
    DenseMap<BasicBlock *, uint64_t> bbid;
    for (auto fnptr:toInstr) {
        auto &fn = *fnptr;
        std::cout << "==================\n"
                  << "function: " << fn.getName().str() << std::endl
                  << "==================\n";
        uint64_t id = 0;
        for (auto &bb: fn) {
            bbid[&bb] = id;
            id++;
        }

        printf("digraph {\n");
        for (auto &bb:fn) {
            for (auto it = succ_begin(&bb), et = succ_end(&bb); it != et; ++it) {
                auto succ = *it;
                auto label = fnEdgeMap[&fn][Edge(&bb, succ)];
                printf("\"%lu\" -> \"%lu\" [label=%lu]\n",
                       bbid[&bb], bbid[succ], label);
            }
        }
        printf("}\n");
    }
}

bool
PathEncodingPass::runOnModule(Module &module) {
    // Add your solution here.
    for (auto &fn:module) {
        if (!fn.isDeclaration()) {
            if (encode(fn)) {
                toInstr.push_back(&fn);
            }
        }
    }
    calcFnId();
    debugPrint();
    return false;
}

void PathEncodingPass::calcFnId() {
    uint64_t i = 0;
    for (auto fn:toInstr) {
        fnId[fn] = i;
        i++;
    }
}

bool
PathEncodingPass::encode(llvm::Function &function) {
    // You may want to write this function as part of your solution
    std::deque<BasicBlock *> worklist;
    DenseSet<BasicBlock *> visited;

    for (auto &bb:function) {
        LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>(function).getLoopInfo();
        auto isLoop = LI.getLoopFor(&bb);
        if (isLoop) {
            return false;
        }
    }

    for (auto &bb:function) {
        for (auto &i:bb) {
            auto result = dyn_cast<CallInst>(&i);
            if (result) {
                worklist.push_back(&bb);
                numPaths[&bb] = 1;
                visited.insert(&bb);
                break;
            }
        }
    }
    //DFS
    while (!worklist.empty()) {
        auto bb = worklist.back();
        worklist.pop_back();
        for (auto it = pred_begin(bb), et = pred_end(bb); it != et; ++it) {
            auto pred = *it;
            if (visited.count(pred)) {
                //cross edge
                numPaths[pred] += numPaths[bb];
            }
            else {
                //discovery edge
                numPaths[pred] = numPaths[bb];
                visited.insert(pred);
                worklist.push_back(pred);
            }
        }
    }

    for (auto &bb:function) {
        uint64_t encoding = 0;
        for (auto it = succ_begin(&bb), et = succ_end(&bb); it != et; ++it) {
            auto succ = *it;
            fnEdgeMap[&function][Edge(&bb, succ)] = encoding;
            encoding += numPaths[succ];
        }
    }
    return true;
}
