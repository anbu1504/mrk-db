#pragma once

#include "constants.hpp"
#include "bufferpool.hpp"

class SSTView {
   private:
    BufferPool* bufPool;
    PageBuffer pageBuf;
    uint64_t sstNum;
    uint64_t pageNum;
    uint64_t pagePos;
    
   public:
    bool checkForKey(uint64_t key);
    void findPage(uint64_t key);
    void fastFwd(uint64_t key);
    uint64_t getCurrKey();
};
