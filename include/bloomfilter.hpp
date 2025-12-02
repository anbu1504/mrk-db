#pragma once

#include "bufferpool.hpp"
#include "constants.hpp"
#include "globals.hpp"

/**
 * @class BloomFilter
 * @brief Probabilistic data structure used to test membership of keys in SSTs.
 */
class BloomFilter {
   private:
    BufferPool* bufPool;
    uint64_t sstNum;
    uint64_t numKeys;
    uint64_t pageOffset;
    uint64_t numHashFunctions;
    uint64_t totalBits;
    std::vector<uint64_t*> pages;

   public:
    /**
     * @brief Constructor for initializing a BloomFilter.
     *
     * @param bufPool Pointer to the buffer pool used for page I/O
     * @param sstNum The SST number the Bloom filter is associated with
     * @param numKeys Number of keys in the SST
     * @param pageOffset The starting page offset for the Bloom filter data
     */
    BloomFilter(BufferPool* bufPool, uint64_t sstNum, uint64_t numKeys, uint64_t pageOffset);

    /**
     * @brief Adds a single key to the Bloom filter.
     *
     * @param key The key to add to the filter
     */
    void addKey(uint64_t key);

    /**
     * @brief Adds multiple keys to the Bloom filter from memtable data.
     *
     * @param memtableData Pointer to a vector storing memtable data in <key, value> order
     */
    void addMultiKeys(std::vector<uint64_t>* memtableData);

    /**
     * @brief Checks if a key may exist in the Bloom filter.
     *
     * @param key The key to check in the filter
     * @return True if the key may be present (false positives possible); false if definitely absent
     */
    bool checkKey(uint64_t key);

    /**
     * @brief Resets all Bloom filter pages to zero.
     */
    void wipePages();

    /**
     * @brief Flushes in-memory Bloom filter pages to disk.
     */
    void flushPages();
};
