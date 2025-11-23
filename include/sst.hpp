#include <functional>

#include <string>
#include "../include/constants.hpp"
#include "../include/mrkdb.hpp"

namespace SST {
    std::tuple<kvPairs, std::vector<uint64_t>> sstSearch(std::vector<uint64_t> keys, int sstNum, sstMetadata metadata,
                                                     bool useBTreeSearch, BufferPool* bufferPool,
                                                     std::string databaseName);

    // // Private helper functions
    // uint64_t getNextBTreeNode(uint64_t currKey, uint64_t* pageBuf);
    // void mergeSort(std::vector<kvPairs>* vectors);
    // int binSearch(int lo, int hi, const std::function<int(int)>& comparator);
}
