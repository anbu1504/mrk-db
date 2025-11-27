#pragma once

#include "constants.hpp"
#include "bufferpool.hpp"
#include "bloomfilter.hpp"
#include "btree.hpp"
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

    uint64_t calcNumItemsInPage(uint64_t pageNum); 
    
   public:
    SSTView(BufferPool* bufPool, uint64_t sstNum);

    bool checkForKey(uint64_t key);
    void findPage(uint64_t key);
    void fastFwd(uint64_t key);
    uint64_t getCurrKey();
};
