#pragma once

#include "bloomfilter.hpp"
#include "btree.hpp"
#include "bufferpool.hpp"
#include "constants.hpp"

/**
 * @class SSTWriter
 * @brief Writes and merges SST files from memtable data.
 */
class SSTWriter {
   private:
    BufferPool* bufPool;
    uint64_t sstNum;

    // Sorts the keys in sstNum1 and sstNum2 and writes them
    // to the file corresponding to sstNum, from page offset 1
    // Returns the number of keys in the new SST
    /**
     * @brief Performs a two-way merge sort of SSTs.
     *
     * @param sstNum1 First SST identifier
     * @param sstNum2 Second SST identifier
     * @return Number of keys in the merged SST
     */
    uint64_t multiwayMergeSort(uint64_t sstNum1, uint64_t sstNum2);

   public:
    /**
     * @brief Constructs an SSTWriter for a target SST.
     *
     * @param bufPool Pointer to the buffer pool used for I/O
     * @param sstNum SST identifier to write to
     */
    SSTWriter(BufferPool* bufPool, uint64_t sstNum);

    /**
     * @brief Writes the smallest sized SST using memtable data.
     *
     * @param memtableData Pointer to memtable data in <key, value> order
     */
    void writeMiniSST(std::vector<uint64_t>* memtableData);

    /**
     * @brief Merges two SSTs into this writer's SST.
     *
     * @param sstNum1 First SST identifier
     * @param sstNum2 Second SST identifier
     */
    void mergeSSTs(uint64_t sstNum1, uint64_t sstNum2);
};
