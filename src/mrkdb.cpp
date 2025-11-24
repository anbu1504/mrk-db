#define _ALL_SOURCE  // Needed for O_DIRECT(?)

#include "../include/mrkdb.hpp"

#include <fcntl.h>  // Also needed for O_DIRECT(?)
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <tuple>    // Needed for std::get
#include <utility>  // Needed for std::swap

#include "../include/sst.hpp"

#define METADATA_FILENAME ".metadata"
#define INITIAL_DIR_SIZE 4
#define MAX_DIR_SIZE 64
#define MAX_NUM_PAGES 4096

int DB::Open(const std::string dbName) {
    databaseName = dbName;
    memtable = new Memtable(THRESHOLD);
    bufferPool = new BufferPool(INITIAL_DIR_SIZE, MAX_DIR_SIZE, MAX_NUM_PAGES);
    sstCount = 0;

    

    if (!std::filesystem::create_directory(dbName)) {  // If the DB already exists
        std::ifstream metadataFile(databaseName + "/" + METADATA_FILENAME);

        if (metadataFile.is_open()) {
            metadataFile >> sstCount;
            metadataFile.close();
        }

        for (int i = 0; i < sstCount; i++) {
            size_t entryCount;
            size_t internalNodeCount;
            uint64_t filterBitCount;
            uint64_t minKey;
            uint64_t maxKey;


            int fd = open(SST_PATH(i).c_str(), O_RDONLY);

            pread(fd, &entryCount, sizeof(entryCount), 0);
            pread(fd, &internalNodeCount, sizeof(internalNodeCount), sizeof(size_t));
            pread(fd, &filterBitCount, sizeof(filterBitCount), sizeof(size_t) * 2);
            pread(fd, &minKey, sizeof(minKey), sizeof(size_t) * 2 + sizeof(uint64_t));
            pread(fd, &maxKey, sizeof(maxKey), sizeof(size_t) * 2 + sizeof(uint64_t) * 2);
                
            sstMetadataCache.push_back(
                std::make_tuple(entryCount, internalNodeCount, filterBitCount, minKey, maxKey));
            close(fd);

        }
    }

    return 0;
};

std::optional<uint64_t> DB::Get(uint64_t key) {
    std::optional<uint64_t> memtableValue = memtable->getValue(key);

    if (memtableValue.has_value()) {
        return memtableValue.value();
    }

    // If we get to this point, then the key doesn't exist in the memtable
    kvPairs sstValues;

    for (int sstNum = sstCount - 1; sstNum >= 0; sstNum--) {
        sstValues = std::get<0>(
            SST::sstSearch({key}, sstNum, sstMetadataCache[sstNum], USE_BTREE_SEARCH, bufferPool, databaseName));
        if (!sstValues.empty()) {
            // Return value (index 1) from first KV-pair (index 0)
            return std::get<1>(sstValues[0]);
        }
    }

    return std::nullopt;
}

kvPairs DB::Scan(uint64_t key1, uint64_t key2) {
    kvPairs memtablePairs = memtable->scanTree(key1, key2);

    if (memtablePairs.size() == (key2 - key1)) {
        return memtablePairs;
    }

    std::vector<uint64_t> keysToFind;
    size_t currIdx = 0;

    for (uint64_t i = key1; i <= key2; i++) {
        if (currIdx == memtablePairs.size() || i < std::get<0>(memtablePairs[currIdx])) {
            keysToFind.push_back(i);
        } else {  // i == memtableValues[currIdx][0]
            currIdx++;
        }
    }

    std::vector<kvPairs> allPairVectors = {
        memtablePairs,
    };

    std::tuple<kvPairs, std::vector<uint64_t>> binSearchRet;

    for (int sstNum = sstCount - 1; sstNum >= 0 && !keysToFind.empty(); sstNum--) {
        binSearchRet =
            SST::sstSearch(keysToFind, sstNum, sstMetadataCache[sstNum], USE_BTREE_SEARCH, bufferPool, databaseName);

        allPairVectors.push_back(std::get<0>(binSearchRet));
        keysToFind = std::get<1>(binSearchRet);
    }

    std::sort(allPairVectors.begin(), allPairVectors.end(), [](kvPairs a, kvPairs b) { return a.size() < b.size(); });

    mergeSort(&allPairVectors);

    return allPairVectors[0];
}

int DB::Put(uint64_t key, uint64_t value) {
    bool success = memtable->insert(key, value);

    if (memtable->isThresholdReached()) {
        std::tuple<size_t, size_t, uint64_t, uint64_t, uint64_t> sstMetadata =
            memtable->flushToDiskBTree(SST_PATH(sstCount));
        sstMetadataCache.push_back(sstMetadata);
        sstCount++;
    }

    return !success;  // 0 for success, 1 for failure
};

int DB::Close() {
    if (!memtable->isEmpty()) {
        memtable->flushToDiskBTree(SST_PATH(sstCount));
        sstCount++;
    }

    delete memtable;

    std::ofstream metadataFile(databaseName + "/" + METADATA_FILENAME);

    if (metadataFile.is_open()) {
        metadataFile << sstCount;
        metadataFile << std::endl;
        metadataFile.close();
    }

    return 0;
};

// In-place merge-sort for a vector of kvPairs (i.e., vector of vectors)
// Does not return anything, and instead replaces all the vectors within the input w/ 1 sorted vector
void DB::mergeSort(std::vector<kvPairs>* vectors) {
    // Temporary variable to help with merge-sort
    std::vector<kvPairs> merged;

    // Greedy iterative 2-way merge-sort
    while (vectors->size() != 1) {
        // Clear and initialize the result vector (merged)
        // w/ the necessary number of placeholders
        merged.assign(CEIL_DIV(vectors->size(), 2), kvPairs());

        for (size_t i = 0; i < merged.size(); i++) {
            if (i * 2 + 1 == vectors->size()) {
                merged[i] = vectors->at(i * 2);
            } else {
                std::merge(vectors->at(i * 2).begin(), vectors->at(i * 2).end(), vectors->at(i * 2 + 1).begin(),
                           vectors->at(i * 2 + 1).end(), std::back_inserter(merged[i]));
            }
        }

        std::swap(*vectors, merged);
    }
}
