#pragma once

#include "memtable.hpp"
#include <cstdint>
#include <string>
#include <tuple>
#include <vector>
#include <filesystem>

// Number of keys, corresponds to flushing 64kb of data to SSTs
// (i.e., 16 pages worth of data) (we plan to also store 1 page for metadata)
#define THRESHOLD 16384

typedef std::vector<std::tuple<uint64_t, uint64_t>> kvPairs;

/**
 * @class DB
 * @brief The MRK-DB database class.
 */
class DB {
    friend class DBTester;
private:
    // Name of the database
    std::string databaseName;

    // Memtable (binary tree)
    Memtable* memtable;

    // Number of SSTs
    int sstCount;

    // For each SST, records a tuple of <numEntries, minKey, maxKey>
    std::vector<std::tuple<size_t, uint64_t, uint64_t>> sstMetadataCache;

    // Private helper functions
    std::tuple<kvPairs, std::vector<uint64_t>> DB::sstBinSearch(std::vector<uint64_t> keys, int sstNum);

public:
    /**
     * @brief Opens the database and prepares it to run.
     * 
     * @param dbName Name of the database to open/create (if it doesn't already exist)
     */
    
    int Open(const std::string dbName);

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
     * @brief Closes the database.
     *
     * @param dbName Name of the database to open
     */
    int Close();
};
