#pragma once

#include "memtable.hpp"
#include <cstdint>
#include <string>
#include <tuple>
#include <vector>
#include <filesystem>

// Number of keys, corresponds to flushing 16kb of data to SSTs
#define threshold 4096

/**
 * @class DB
 * @brief The MRK-DB database class.
 */
class DB {
private:
    std::string databaseName;
    Memtable* memtable;
    int sstCount;

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
    uint64_t Get(uint64_t key);

    /**
     * @brief Retrieves all KV-pairs in a key range in key order (key1 < key2).
     *
     * @param key1 The first key in the key range for which values will be retrieved
     * @param key1 The last key in the key range for which values will be retrieved
     */
     std::vector<std::tuple<uint64_t, uint64_t>> Scan(uint64_t key1, uint64_t key2);

    /**
     * @brief Closes the database.
     *
     * @param dbName Name of the database to open
     */
    int Close();
};
