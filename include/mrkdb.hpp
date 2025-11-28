#pragma once

#include "bufferpool.hpp"
#include "constants.hpp"
#include "globals.hpp"
#include "lsmtree.hpp"

/**
 * @class DB
 * @brief The MRK-DB database class.
 */
class DB {
    friend class DBTester;

   private:
    // Name of the database
    std::string dbName;

    LSMTree* lsmTree;

    // Bufferpool
    BufferPool* bufPool;

   public:
    /**
     * @brief Opens the database and prepares it to run.
     *
     * @param dbName Name of the database to open/create (if it doesn't already exist)
     */

    int Open(const std::string dbName, bool useBTreeSearchValue, uint64_t bitsPerEntryValue,
             uint64_t initialDirSizeValue, uint64_t maxDirSizeValue, uint64_t maxNumPagesValue,
             uint64_t memtableThresholdValue);

    /**
     * @brief Stores a key associated with a value.
     *
     * @param key The key to be stored
     * @param value The value to be associated with the key
     */
    int Put(uint64_t key, uint64_t value);

    /**
     * @brief Retrieves a value associated with a given key.
     *
     * @param key The key for which the associated value will be retrieved
     */
    std::optional<uint64_t> Get(uint64_t key);

    /**
     * @brief Retrieves all KV-pairs in a key range in key order (key1 < key2).
     *
     * @param key1 The first key in the key range for which values will be retrieved
     * @param key1 The last key in the key range for which values will be retrieved
     */
    kvPairs Scan(uint64_t key1, uint64_t key2);

    /**
     * @brief Deletes a key-value pair based on the key
     *
     * @param key The key in question who's key value pair should be deleted
     */

    int Delete(uint64_t key);

    /**
     * @brief Closes the database.
     *
     * @param dbName Name of the database to open
     */
    int Close();
};
