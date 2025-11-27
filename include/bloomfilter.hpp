#pragma once

#include "constants.hpp"
#include "bufferpool.hpp"

class BloomFilter {
   private:
    BufferPool* bufPool;
    uint64_t sstNum;
    uint64_t numKeys;
    uint64_t pageOffset;
    uint64_t bitsPerEntry; // M
    uint64_t numHashFunctions;
    uint64_t totalBits;

   public:
    BloomFilter(BufferPool* bufPool, uint64_t sstNum, uint64_t numKeys, uint64_t pageOffset, uint64_t bitsPerEntry);
    ~BloomFilter();

    void addKey(uint64_t key);
    void addMultiKeys(uint64_t* memtableData);
    bool checkKey(uint64_t key);
};
