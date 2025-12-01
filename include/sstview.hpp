#pragma once

#include "bloomfilter.hpp"
#include "btree.hpp"
#include "bufferpool.hpp"
#include "constants.hpp"
#include "globals.hpp"

class SSTView {
   private:
    BufferPool* bufPool;
    uint64_t sstNum;

    // Metadata
    uint64_t numKeys;
    uint64_t minKey;
    uint64_t maxKey;

    // Used for scans
    PageBuffer pageBuf;
    uint64_t keysRead;
    uint64_t pageNum;
    uint64_t currKeyIdx;

   public:
    SSTView(BufferPool* bufPool, uint64_t sstNum);

    bool checkForKey(uint64_t key);
    void findPage(uint64_t key);
    void fastFwd(uint64_t key);
    uint64_t getCurrKey();
    uint64_t getCurrValue();
};
