#include "../include/globals.hpp"

// initialize global variables
bool useBTreeSearch = true;
uint64_t bitsPerEntry = 8;
// uint64_t initialDirSize = 4;
// uint64_t maxDirSize = 64;
uint64_t cacheSize = 2560;           // this is 10MB worth of cached pages
uint64_t memtableThreshold = 65536;  // this is 1MB worth of entries
