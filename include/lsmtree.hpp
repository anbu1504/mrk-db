#pragma once

#include "bufferpool.hpp"
#include "constants.hpp"
#include "memtable.hpp"
#include "sstview.hpp"
#include "sstwriter.hpp"

#define SCALE_FACTOR 2

/**
 * @class LSMTree
 * @brief Log-structured merge tree coordinating memtable flushes and SSTs.
 */
class LSMTree {
   private:
    BufferPool* bufPool;
    Memtable* memtable;

    std::vector<uint64_t> levels;
    uint64_t scaleFactor;  // basically M (needs to be a constant after)

    /**
     * @brief Handles flushing the memtable to disk and creating SSTs.
     */
    void flushHelper();

   public:
    /**
     * @brief Constructs an LSMTree with default level sizing.
     *
     * @param bufPool Pointer to the buffer pool used for I/O
     */
    LSMTree(BufferPool* bufPool);

    /**
     * @brief Constructs an LSMTree with a custom level configuration.
     *
     * @param bufPool Pointer to the buffer pool used for I/O
     * @param levels Vector describing SST counts per level
     */
    LSMTree(BufferPool* bufPool, std::vector<uint64_t> levels);

    /**
     * @brief Frees resources and flushes remaining data.
     */
    ~LSMTree();

    /**
     * @brief Inserts or updates a key-value pair.
     *
     * @param key Key to insert
     * @param value Value associated with the key
     */
    void Put(uint64_t key, uint64_t value);

    /**
     * @brief Retrieves a value for the given key.
     *
     * @param key Key to search for
     */
    uint64_t Get(uint64_t key);

    /**
     * @brief Scans a key range across all levels.
     *
     * @param key1 Inclusive start key
     * @param key2 Inclusive end key
     */
    kvPairs Scan(uint64_t key1, uint64_t key2);

    /**
     * @brief Closes the tree, flushing data and returning SST identifiers.
     *
     * @return Vector of SST numbers generated on close
     */
    std::vector<uint64_t> Close();

    /**
     * @brief Returns current occupancy per level.
     */
    std::vector<uint64_t> getOccupancyLevels();
};
