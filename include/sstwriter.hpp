#pragma once

#include "constants.hpp"
#include "bufferpool.hpp"

class SSTWriter {
   private:
    BufferPool* bufPool;
    uint64_t sstNum;

    void multiwayMergeSort(int sstNum1, int sstNum2);
    
   public:
    void writeMiniSST(std::vector<uint64_t>* memtableData);
    void mergeSSTs(uint64_t sstNum1, uint64_t sstNum2);
};
