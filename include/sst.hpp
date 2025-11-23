#include <functional>

#include <string>
#include "../include/constants.hpp"
#include "../include/mrkdb.hpp"

namespace SST {
    std::tuple<kvPairs, std::vector<uint64_t>> sstSearch(std::vector<uint64_t> keys, int sstNum, sstMetadata metadata,
                                                     bool useBTreeSearch, BufferPool* bufferPool,
                                                     std::string databaseName);

    sstMetadata sstWrite(std::string filename, std::vector<uint64_t>* memtable_data, size_t flushed_size);
}
