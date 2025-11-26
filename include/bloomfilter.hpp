#pragma once

#include "constants.hpp"
#include "bufferpool.hpp"

class BloomFilter {
   private:
    BufferPool* bufPool;
    uint64_t sstNum;
    uint64_t numKeys;
    uint64_t pageOffset;

   public:
    BloomFilter(uint64_t sstNum, uint64_t numKeys, uint64_t pageOffset);
    ~BloomFilter();

    void addKey(uint64_t key);
    void addMultiKeys(uint64_t* memtableData);
    bool checkKey(uint64_t key);
};
