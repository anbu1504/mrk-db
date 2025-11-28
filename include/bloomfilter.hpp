#pragma once

#include "bufferpool.hpp"
#include "constants.hpp"
#include "globals.hpp"

class BloomFilter {
   private:
    BufferPool* bufPool;
    uint64_t sstNum;
    uint64_t numKeys;
    uint64_t pageOffset;
    uint64_t numHashFunctions;
    uint64_t totalBits;

   public:
    BloomFilter(BufferPool* bufPool, uint64_t sstNum, uint64_t numKeys, uint64_t pageOffset);

    void addKey(uint64_t key);
    void addMultiKeys(std::vector<uint64_t>* memtableData);
    bool checkKey(uint64_t key);
    void wipePages();
};
