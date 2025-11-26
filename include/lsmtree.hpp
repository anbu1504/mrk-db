#pragma once

#include "bufferpool.hpp"
#include "constants.hpp"
#include "memtable.hpp"
#include "sstview.hpp"
#include "sstwriter.hpp"

class LSMTree {
   private:
    BufferPool* bufPool;
    Memtable* memtable;

    std::vector<uint64_t> levels;
    uint64_t numLevels;
    uint64_t scaleFactor; // basically M (needs to be a constant after)

    int compaction(uint64_t sstNum1, uint64_t sstNum2);

   public:
    LSMTree(BufferPool* bufPool, uint64_t numLevelsValue, uint64_t scaleFactorValue);
    ~LSMTree();

    void Put(uint64_t key, uint64_t value);
    uint64_t Get(uint64_t key);
    kvPairs Scan(uint64_t key1, uint64_t key2);
};
