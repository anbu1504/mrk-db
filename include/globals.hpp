#pragma once

#include <cstdint>

/** @brief Enables B-Tree based search when true; defaults to binary search otherwise. */
extern bool useBTreeSearch;

/** @brief Bits per entry used for Bloom filters. */
extern uint64_t bitsPerEntry;

/** @brief Maximum number of cached pages in the buffer pool. */
extern uint64_t cacheSize;

/** @brief Threshold at which the memtable flushes to disk. */
extern uint64_t memtableThreshold;
