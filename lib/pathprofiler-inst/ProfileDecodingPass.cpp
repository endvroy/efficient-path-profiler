

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FileSystem.h"

#include <unordered_map>
#include <queue>
#include "ProfileDecodingPass.h"
#include <unistd.h>

using namespace llvm;
using namespace pathprofiling;


namespace pathprofiling {
    char ProfileDecodingPass::ID = 0;
}

typedef std::pair<uint64_t, uint64_t> Info;

// Given a sequence of basic blocks composing a path, this function prints
// out the filename and line numbers associated with that path in CSV format.
void
printPath(FILE *f, std::vector<llvm::BasicBlock *> &blocks) {
    unsigned line = 0;
    llvm::StringRef file;
    for (auto *bb : blocks) {
        for (auto &instruction : *bb) {
            llvm::DILocation *loc = instruction.getDebugLoc();
            if (loc && (loc->getLine() != line || loc->getFilename() != file)) {
                line = loc->getLine();
                file = loc->getFilename();
//                out << ", " << file.str() << ", " << line;
                fprintf(f, ", %s, %u", file.str().c_str(), line);
            }
        }
    }
}


bool
ProfileDecodingPass::runOnModule(Module &module) {
    // Implement your solution here.
//    outs() << infilename.str().c_str();
    auto inf = fopen(infilename.str().c_str(), "r");
    uint64_t fnId, pathCode, count;
    std::vector<uint64_t> countVec;
    std::vector<Info> infoVec;

    char *line = NULL;
    size_t len = 0;
    while (getline(&line, &len, inf) != -1) {
        sscanf(line, "%lu %lu %lu", &fnId, &pathCode, &count);
//        printf("read line: %lu %lu %lu\n", fnId, pathCode, count);
        countVec.push_back(count);
        infoVec.emplace_back(fnId, pathCode);
    }
    fclose(inf);

//    outs() << "\nreading file complete\n";

    std::vector<int> toPrint;
    uint64_t k;
    std::priority_queue<std::pair<double, int>> q;
    for (int i = 0; i < countVec.size(); i++) {
        q.push(std::pair<uint64_t, int>(countVec[i], i));
    }
    if (countVec.size() <= 5) {
        k = countVec.size();
    }
    else {
        k = 5;
    }
    for (int i = 0; i < k; i++) {
        auto idx = q.top().second;
        q.pop();
        toPrint.push_back(idx);
    }


    auto of = fopen("top-five-paths.csv", "w");
    auto &ep = getAnalysis<PathEncodingPass>();
    for (auto idx:toPrint) {
        count = countVec[idx];
        auto info = infoVec[idx];
        fnId = info.first;
        pathCode = info.second;
        auto fn = ep.toInstr[fnId];
        auto seq = decode(fn, pathCode);
        fprintf(of, "%lu, %s", count, fn->getName().str().c_str());
        printPath(of, seq);
        fprintf(of, "\n");
    }
    fclose(of);

    return false;
}


std::vector<llvm::BasicBlock *>
ProfileDecodingPass::decode(llvm::Function *function, uint64_t pathCode) {
    std::vector<llvm::BasicBlock *> sequence;
    // You may want to implement and use this function as a part of your
    // solution.

    using Edge = std::pair<llvm::BasicBlock *, llvm::BasicBlock *>;
    auto &ep = getAnalysis<PathEncodingPass>();
    auto &allEdges = ep.fnEdgeMap[function];

    auto bb = &(function->getEntryBlock());
    BasicBlock *next = nullptr;
    while (true) {
        sequence.push_back(bb);
        uint64_t encoding = 0;
        next = nullptr;
        for (auto it = succ_begin(bb), et = succ_end(bb); it != et; it++) {
            auto succ = *it;
            auto edgeCode = allEdges[Edge(bb, succ)];
            if (edgeCode >= encoding && edgeCode <= pathCode) {
                encoding = edgeCode;
                next = succ;
            }
        }
        if (next) {
            pathCode -= encoding;
            bb = next;
        }
        else {
            break;  //no successor
        }
    }

    return sequence;
}

