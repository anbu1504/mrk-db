#pragma once

#include "bloomfilter.hpp"
#include "btree.hpp"
#include "bufferpool.hpp"
#include "constants.hpp"
#include "globals.hpp"

/**
 * @class SSTView
 * @brief Provides read-only access to an SST for point and range queries.
 */
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
    /**
     * @brief Constructs an SSTView over a given SST
     *
     * @param bufPool Pointer to the buffer pool used for I/O
     * @param sstNum SST identifier to view
     */
    SSTView(BufferPool* bufPool, uint64_t sstNum);

    /**
     * @brief Checks whether the SST may contain the key using Bloom filter and B-Tree.
     *
     * @param key Key to check
     */
    bool checkForKey(uint64_t key);

    /**
     * @brief Finds and loads the page that should contain the key.
     *
     * @param key Key to locate
     */
    void findPage(uint64_t key);

    /**
     * @brief Advances the internal cursor to the key or next greater key.
     *
     * @param key Key to advance to
     */
    void fastFwd(uint64_t key);

    /**
     * @brief Returns the current key at the cursor.
     */
    uint64_t getCurrKey();

    /**
     * @brief Returns the current value at the cursor.
     */
    uint64_t getCurrValue();
};
