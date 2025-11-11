#define _ALL_SOURCE // Needed for O_DIRECT(?)

#include "../include/mrkdb.hpp"
#include <algorithm>
#include <filesystem>
#include <fcntl.h> // Also needed for O_DIRECT(?)
#include <fstream>
#include <unistd.h>
#include <utility> // Needed for std::swap

#define METADATA_FILENAME ".metadata"
#define PAGE_SIZE 4096
#define CEIL_DIV(x, y) ((x) / (y) + ((x) % (y) != 0))
#define SST_PATH(x) (databaseName + "/" + std::to_string(x) + ".sst")
// #define PRINT(x) (std::cout << x << std::endl)


int DB::Open(const std::string dbName) {
    databaseName = dbName;
    memtable = new Memtable(THRESHOLD);
    sstCount = 0;

    if (!std::filesystem::create_directory(dbName)) { // If the DB already exists
        std::ifstream metadataFile(databaseName + "/" + METADATA_FILENAME);

        if (metadataFile.is_open()) {
            metadataFile >> sstCount;
            metadataFile.close();
        }

        for (int i = 0; i < sstCount; i++) {
            size_t entryCount;
            size_t internalNodeCount;
            uint64_t minKey;
            uint64_t maxKey;
            std::ifstream sstFile(SST_PATH(i));

            if (sstFile.is_open()) {
                sstFile >> entryCount;
                sstFile >> internalNodeCount;
                sstFile >> minKey;
                sstFile >> maxKey;
                sstMetadataCache.push_back(std::make_tuple(entryCount, internalNodeCount, minKey, maxKey));
                sstFile.close();
            } else {
                return 1; // Error: This SST should exist
            }
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
        sstValues = std::get<0>(sstBinSearch({key, }, sstNum));
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
    int currIdx = 0;

    for (uint64_t i = key1; i <= key2; i++) {
        if (currIdx == memtablePairs.size() || i < std::get<0>(memtablePairs[currIdx])) {
            keysToFind.push_back(i);
        } else { // i == memtableValues[currIdx][0]
            currIdx++;
        }
    }

    std::vector<kvPairs> allPairVectors = {memtablePairs, };

    std::tuple<kvPairs, std::vector<uint64_t>> binSearchRet;

    for (int sstNum = sstCount - 1; sstNum >= 0 && !keysToFind.empty(); sstNum--) {
        binSearchRet = sstBinSearch(keysToFind, sstNum);

        allPairVectors.push_back(std::get<0>(binSearchRet));
        keysToFind = std::get<1>(binSearchRet);
    }

    std::sort(allPairVectors.begin(), allPairVectors.end(), [](kvPairs a, kvPairs b) {
        return a.size() < b.size();
    });

    mergeSort(&allPairVectors);

    return allPairVectors[0];
}

int DB::Put(uint64_t key, uint64_t value) {
    bool success = memtable->insert(key, value);

    if (memtable->isThresholdReached()) {
        std::tuple<size_t, size_t, uint64_t, uint64_t> sstMetadata = memtable->flushToDiskBTree(SST_PATH(sstCount));
        sstMetadataCache.push_back(sstMetadata);
        sstCount++;
    }

    return !success; // 0 for success, 1 for failure
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

// Binary search in the SST corresponding to sstNum, to find the provided keys
// Returns a tuple of: (1) Vector of KV pairs that were found in the SST, and (2) Keys that weren't found in the SST
std::tuple<kvPairs, std::vector<uint64_t>> DB::sstBinSearch(std::vector<uint64_t> keys, int sstNum) {
    kvPairs foundPairs;
    std::vector<uint64_t> keysNotFound;
    std::vector<uint64_t> keysToFind;

    auto [entryCount, internalNodeCount, minKey, maxKey] = sstMetadataCache[sstNum];

    // Filter out keys that are outside the range of this SST
    for (int i = 0; i < keys.size(); i++) {
        if (keys[i] < minKey || keys[i] > maxKey) {
            keysNotFound.push_back(keys[i]);
        } else {
            keysToFind.push_back(keys[i]);
        }
    }

    // If we have no keys to look for in this SST, then no need to do any I/O here
    if (keysToFind.empty()) {
        return std::make_tuple(foundPairs, keysNotFound);
    }

    // Reverse our keysToFind list, since popping from the back is O(1)
    std::reverse(keysToFind.begin(), keysToFind.end());


    uint64_t currKey = keysToFind.back();
    int fd = open(SST_PATH(sstNum).c_str(), O_RDONLY); // | O_DIRECT);

    // Division to obtain number of pages, rounded UP to nearest whole num
    // We multiply entryCount by 2 because there's a Key and Value for each "entry"
    int numPages = CEIL_DIV(entryCount * 2, PAGE_SIZE);

    int lo = 1 + internalNodeCount;
    int hi = numPages;
    int mid;

    uint64_t pageBuf[PAGE_SIZE / sizeof(uint64_t)];
    ssize_t bytesRead;
    int itemsRead;

    // Binary search to find the correct page
    while (lo <= hi) {
        mid = lo + (hi - lo) / 2;

        bytesRead = pread(fd, pageBuf, PAGE_SIZE, PAGE_SIZE * mid); // assert(bytesRead > 0)
        itemsRead = bytesRead / sizeof(uint64_t);

        if (currKey < pageBuf[0]) {
            hi = mid - 1;
        } else if (currKey > pageBuf[itemsRead - 2]) {
            lo = mid + 1;
        } else {
            break; // We found the page that would contain the first key
        }
    }

    int keysRead = itemsRead / 2;

    lo = 0;
    hi = keysRead - 1;
    int mid2;
    uint64_t midKey;

    // Binary search to find the correct key within pageBuf
    while (lo <= hi) {
        mid2 = lo + (hi - lo) / 2;

        midKey = pageBuf[mid2 * 2];

        if (currKey < midKey) {
            hi = mid2 - 1;
        } else if (currKey > midKey) {
            lo = mid2 + 1;
        } else {
            break;
        }
    }

    // At this point, mid2 is either equal to the index of currKey itself,
    // or the next smallest key after currKey (if currKey wasn't found)

    while (!keysToFind.empty()) {
        currKey = keysToFind.back();
        midKey = pageBuf[mid2 * 2];

        while (midKey < currKey) {
            mid2++;

            // If mid2 is out of bounds, read next page and set mid2 to 0
            if (mid2 >= keysRead) {
                mid++;
                bytesRead = pread(fd, pageBuf, PAGE_SIZE, PAGE_SIZE * mid);
                itemsRead = bytesRead / sizeof(uint64_t);
                keysRead = itemsRead / 2;

                mid2 = 0;
            }

            midKey = pageBuf[mid2 * 2];
        }

        // Now, midKey >= currKey
        if (midKey == currKey) {
            foundPairs.push_back(std::make_tuple(midKey, pageBuf[mid2 * 2 + 1]));
        } else {
            keysNotFound.push_back(currKey);
        }

        keysToFind.pop_back();
    }

    return std::make_tuple(foundPairs, keysNotFound);
}

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

        for (int i = 0; i < merged.size(); i++) {
            if (i*2 + 1 == vectors->size()) {
                merged[i] = vectors->at(i*2);
            } else {
                std::merge(
                    vectors->at(i*2).begin(), vectors->at(i*2).end(),
                    vectors->at(i*2 + 1).begin(), vectors->at(i*2 + 1).end(),
                    std::back_inserter(merged[i])
                );
            }
        }

        std::swap(*vectors, merged);
    }
}
