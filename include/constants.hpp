#pragma once

#include <tuple>
#include <vector>
#include <cstdint>
#include <optional>
#include <string>

#define PAGE_SIZE 4096
#define UINT64_SIZE 8
#define UINT64S_PER_PAGE 512
#define LN_2 0.69
#define TOMBSTONE UINT64_MAX // source: project doc

#define CEIL_DIV(x, y) ((x) / (y) + ((x) % (y) != 0))
#define PRINT(x) (std::cout << x << std::endl)
#define SST_PATH(x) (databaseName + "/" + std::to_string(x) + ".sst")


typedef std::tuple<size_t, size_t, uint64_t, uint64_t, uint64_t> sstMetadata;
typedef std::vector<std::tuple<uint64_t, uint64_t>> kvPairs;
typedef uint64_t PageBuffer[UINT64S_PER_PAGE];
