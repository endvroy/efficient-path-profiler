
#include <cstdint>
#include <cstdio>
#include <map>

extern "C" {


// This macro allows us to prefix strings so that they are less likely to
// conflict with existing symbol names in the examined programs.
// e.g. EPP(entry) yields PaThPrOfIlInG_entry
#define EPP(X)  PaThPrOfIlInG_ ## X

// Implement your instrumentation functions here. You will probably need at
// least one function to log completed paths and one function to save the
// results to a file. You may wish to have others.
typedef std::pair<uint64_t, uint64_t> Key;
typedef std::map<Key, uint64_t> Counter;

Counter *counterPtr;

void PaThPrOfIlInG_init() {
    counterPtr = new Counter;
}

void PaThPrOfIlInG_count(uint64_t fnId, uint64_t pathCode) {
//    printf("fn: %lu, path :%lu\n", fnId, pathCode);
    auto &counter = *counterPtr;
    auto key = Key(fnId, pathCode);
    if (counter.find(key) == counter.end()) {
        counter[key] = 1;
    }
    else {
        counter[key]++;
    }
}

void PaThPrOfIlInG_print() {
    auto file = fopen("path-profile-results", "w");
    auto &counter = *counterPtr;
    for (auto p:counter) {
        auto x = p.first;
        auto count = p.second;
        auto fnId = x.first;
        auto pathCode = x.second;
        fprintf(file, "%lu %lu %lu\n", fnId, pathCode, count);
    }
}

}

