#pragma once

#include <tuple>
#include <vector>
#include <cstdint>
#include <optional>
#include <string>
#include <functional>

#define PAGE_SIZE 4096
#define UINT64_SIZE 8
#define UINT64S_PER_PAGE 512
#define LN_2 0.69
#define TOMBSTONE UINT64_MAX // source: project doc
#define UINT64_SIZE_BITS 64

#define CEIL_DIV(x, y) ((x) / (y) + ((x) % (y) != 0))
#define PRINT(x) (std::cout << x << std::endl)
#define SST_PATH(x) (databaseName + "/" + std::to_string(x) + ".sst")
#define CALC_NUM_PAGES(num_items, item_size) (CEIL_DIV((num_items) * (item_size), PAGE_SIZE))
#define SST_TEMP_NUM(s) (UINT64_MAX - 1 - (s))


typedef std::tuple<size_t, size_t, uint64_t, uint64_t, uint64_t> sstMetadata;
typedef std::vector<std::tuple<uint64_t, uint64_t>> kvPairs;
typedef uint64_t PageBuffer[UINT64S_PER_PAGE];

// This function must ONLY be called if the desired values is in [lo, hi]
uint64_t binSearch(uint64_t lo, uint64_t hi, const std::function<int(uint64_t)>& comparator) {
    uint64_t mid;

    while (lo <= hi) {
        mid = lo + (hi - lo) / 2;

        int direction = comparator(mid);

        if (direction < 0) {
            hi = mid - 1;
        } else if (direction > 0) {
            lo = mid + 1;
        } else {
            break;
        }
    }

    return mid;
}
