#pragma once

#include "bloomfilter.hpp"
#include "btree.hpp"
#include "bufferpool.hpp"
#include "constants.hpp"

class SSTWriter {
   private:
    BufferPool* bufPool;
    uint64_t sstNum;

    void multiwayMergeSort(int sstNum1, int sstNum2);

   public:
    SSTWriter(BufferPool* bufPool, uint64_t sstNum);
    void writeMiniSST(std::vector<uint64_t>* memtableData);
    void mergeSSTs(uint64_t sstNum1, uint64_t sstNum2);
};
