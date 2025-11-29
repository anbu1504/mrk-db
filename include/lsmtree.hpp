#pragma once

#include "bufferpool.hpp"
#include "constants.hpp"
#include "memtable.hpp"
#include "sstview.hpp"
#include "sstwriter.hpp"

#define SCALE_FACTOR 2

class LSMTree {
   private:
    BufferPool* bufPool;
    Memtable* memtable;

    std::vector<uint64_t> levels;
    uint64_t scaleFactor;  // basically M (needs to be a constant after)

    void compaction(uint64_t sstNum1, uint64_t sstNum2);
    void flushHelper();

   public:
    LSMTree(BufferPool* bufPool);
    LSMTree(BufferPool* bufPool, std::vector<uint64_t> levels);
    ~LSMTree();

    void Put(uint64_t key, uint64_t value);
    uint64_t Get(uint64_t key);
    kvPairs Scan(uint64_t key1, uint64_t key2);
    void Close();
    std::vector<uint64_t> getOccupancyLevels();
};
