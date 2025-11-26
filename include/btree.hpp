#pragma once

#include "constants.hpp"
#include "bufferpool.hpp"

class BTree {
   private:
    BufferPool* bufPool;
    uint64_t sstNum;
    uint64_t numKeys;
    uint64_t pageOffset;

   public:
    BTree(uint64_t sstNum, uint64_t numKeys, uint64_t pageOffset);
    ~BTree();

    uint64_t findLeafPage(uint64_t key);
    uint64_t createFromMem(std::vector<uint64_t>* memtableData);
    uint64_t createFromDisk();
};
