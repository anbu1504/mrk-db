#pragma once

#include "bloomfilter.hpp"
#include "btree.hpp"
#include "bufferpool.hpp"
#include "constants.hpp"

class SSTWriter {
   private:
    BufferPool* bufPool;
    uint64_t sstNum;

    // Sorts the keys in sstNum1 and sstNum2 and writes them
    // to the file corresponding to sstNum, from page offset 1
    // Returns the number of keys in the new SST
    uint64_t multiwayMergeSort(uint64_t sstNum1, uint64_t sstNum2);

   public:
    SSTWriter(BufferPool* bufPool, uint64_t sstNum);
    void writeMiniSST(std::vector<uint64_t>* memtableData);
    void mergeSSTs(uint64_t sstNum1, uint64_t sstNum2);
};
