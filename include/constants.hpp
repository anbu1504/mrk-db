#pragma once

#include <stdio.h>

#include <cstdint>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#define LN_2 0.69
#define PAGE_SIZE 4096
#define UINT64_SIZE 8
#define UINT64_SIZE_BITS 64
#define UINT64S_PER_PAGE 512
#define TOMBSTONE UINT64_MAX  // source: project doc

#define CEIL_DIV(x, y) ((x) / (y) + ((x) % (y) != 0))
#define PRINT(x) (std::cout << x << std::endl)
#define CALC_NUM_PAGES(num_items, item_size) (CEIL_DIV((num_items) * (item_size), PAGE_SIZE))

// Conversion to SST numbers
#define METADATA_NUM (UINT64_MAX - 1)
#define LAYER_DATA_NUM (UINT64_MAX - 2)
#define SST_TEMP_NUM(s) (UINT64_MAX - 3 - (s))

typedef std::tuple<size_t, size_t, uint64_t, uint64_t, uint64_t> sstMetadata;
typedef std::vector<std::tuple<uint64_t, uint64_t>> kvPairs;
typedef uint64_t PageBuffer[UINT64S_PER_PAGE];

/**
 * @brief Binary search with custom comparator.
 *
 * @param lo Lower bound index (inclusive)
 * @param hi Upper bound index (inclusive)
 * @param comparator Function returning -1, 0, or 1 to guide search
 */
uint64_t binSearch(uint64_t lo, uint64_t hi, const std::function<int(uint64_t)>& comparator);

/**
 * @brief Calculates how many items are in a given page if the items are stored contiguously.
 *
 * @param numKeys Total number of keys in the SST
 * @param pageNum Page number being queried
 */
uint64_t calcNumItemsInPage(uint64_t numKeys, uint64_t pageNum);
