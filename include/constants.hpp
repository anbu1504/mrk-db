#pragma once

#include <cstdint>

#define PAGE_SIZE 4096
#define CEIL_DIV(x, y) ((x) / (y) + ((x) % (y) != 0))
#define PRINT(x) (std::cout << x << std::endl)
#define SST_PATH(x) (databaseName + "/" + std::to_string(x) + ".sst")
#define UINT64S_PER_PAGE (PAGE_SIZE / sizeof(uint64_t))
#define TOMBSTONE UINT64_MAX // source: project doc

typedef std::tuple<size_t, size_t, uint64_t, uint64_t, uint64_t> sstMetadata;
typedef std::vector<std::tuple<uint64_t, uint64_t>> kvPairs;
