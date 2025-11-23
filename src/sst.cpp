#include <functional>

#include "../include/sst.hpp"
#include "../include/bloomfilter.hpp"
#include "../include/bufferpool.hpp"

namespace SST {

int binSearch(int lo, int hi, const std::function<int(int)>& comparator) {
    int mid;

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

uint64_t getNextBTreeNode(uint64_t currKey, uint64_t* pageBuf) {
    uint64_t numKeysInNode = pageBuf[0];
    uint64_t startOfChildren = 1 + numKeysInNode;

    // If the key we're looking for is larger than the last delimiting
    // key, then we can just immediately go down to the rightmost child
    if (currKey > pageBuf[numKeysInNode]) {
        return pageBuf[startOfChildren + numKeysInNode];
    }

    // LINEAR SEARCH

    // // Otherwise, we know that currKey must be less than (or equal to)
    // // one of the delimiting keys in this node, which we must find
    // for (uint64_t delimKeyIdx = 0; delimKeyIdx < numKeysInNode; delimKeyIdx++) {
    //     if (currKey <= pageBuf[1 + delimKeyIdx]) {
    //         currPage = pageBuf[startOfChildren + delimKeyIdx];
    //         break;
    //     }
    // }

    // BINARY SEARCH

    // If the key we're looking for is smaller than/equall to the first delimiting
    // key, then we can just immediately go down to the leftmost child
    if (currKey <= pageBuf[1]) {
        return pageBuf[startOfChildren];
    }

    int lo = 1;                  // Corresponds to the second key (we alr. checked for left child of the first key)
    int hi = numKeysInNode - 1;  // Index of last key (we alr. checked for right child of the last key)

    // Note, pageBuf[1 + mid] is the key we're currently inspecting
    // (+1 for offset), while pageBuf[mid] is the key before it
    int mid = binSearch(lo, hi, [&](int m) {
        return (currKey <= pageBuf[m]) ? -1 : (currKey > pageBuf[1 + m]) ? 1 : 0;
    });  // Else case: pageBuf[mid] < currKey && currKey <= pageBuf[1 + mid]

    return pageBuf[startOfChildren + mid];
}

// B-Tree/Binary (depending on USE_BTREE_SEARCH) search in the SST corresponding to sstNum, to find the provided keys
// Returns a tuple of: (1) Vector of KV pairs that were found in the SST, and (2) Keys that weren't found in the SST
std::tuple<kvPairs, std::vector<uint64_t>> sstSearch(std::vector<uint64_t> keys, int sstNum, sstMetadata metadata,
                                                     bool useBTreeSearch, BufferPool* bufferPool,
                                                     std::string databaseName) {
    kvPairs foundPairs;
    std::vector<uint64_t> keysNotFound;
    std::vector<uint64_t> keysToFind;

    auto [entryCount, internalNodeCount, filterBitCount, minKey, maxKey] = metadata;

    BloomFilter filter(filterBitCount, 0);
    uint64_t filterPageCount = filter.getNumPages();

    uint64_t filterBuf[PAGE_SIZE / sizeof(uint64_t)];
    // Initialize the bloom filter (starts at 1 to account for metadata page)
    for (size_t pageNum = 1; pageNum <= filterPageCount; pageNum++) {
        bufferPool->comboRead(SST_PATH(sstNum), sstNum, PAGE_SIZE * pageNum, filterBuf, PAGE_SIZE);
        filter.initFromBuf(filterBuf);
    }

    // Filter out keys that are either outside the range of this SST, or not in the bloom filter
    for (size_t i = 0; i < keys.size(); i++) {
        if (keys[i] < minKey || keys[i] > maxKey || !filter.checkKey(keys[i])) {
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
    // int fd = open(SST_PATH(sstNum).c_str(), O_RDONLY); // | O_DIRECT);

    // Variables for page reads
    uint64_t pageBuf[PAGE_SIZE / sizeof(uint64_t)];
    ssize_t bytesRead;
    int itemsRead;

    int candidatePageNum;  // The page in which we want to look for currKey

    if (useBTreeSearch) {
        // B-Tree search to find the correct page
        uint64_t currPage = 1 + filterPageCount;  // page corresponding to root node

        // Keep going until we reach a leaf node (leaf nodes start at page #(1 + filterPageCount + internalNodeCount))
        while (currPage < 1 + filterPageCount + internalNodeCount) {
            bufferPool->comboRead(SST_PATH(sstNum), sstNum, PAGE_SIZE * currPage, pageBuf, PAGE_SIZE);
            // pread(fd, pageBuf, PAGE_SIZE, PAGE_SIZE * currPage);
            currPage = getNextBTreeNode(currKey, pageBuf);
        }
        candidatePageNum = currPage;
        bytesRead = bufferPool->comboRead(SST_PATH(sstNum), sstNum, PAGE_SIZE * currPage, pageBuf, PAGE_SIZE);
        // bytesRead = pread(fd, pageBuf, PAGE_SIZE, PAGE_SIZE * currPage);
        itemsRead = bytesRead / sizeof(uint64_t);

    } else {
        // Division to obtain number of pages, rounded UP to nearest whole num
        // We multiply entryCount by 2 because there's a Key and Value for each "entry"
        int numLeafPages = CEIL_DIV(entryCount * 2, PAGE_SIZE);

        // Binary search variables
        int lo = 1 + filterPageCount + internalNodeCount;
        int hi = filterPageCount + internalNodeCount + numLeafPages;

        candidatePageNum = binSearch(lo, hi, [&](int m) {
            bytesRead = bufferPool->comboRead(SST_PATH(sstNum), sstNum, PAGE_SIZE * m, pageBuf, PAGE_SIZE);
            itemsRead = bytesRead / sizeof(uint64_t);

            return (currKey < pageBuf[0]) ? -1 : (currKey > pageBuf[itemsRead - 2]) ? 1 : 0;
        });
    }

    // At this point, pageBuf should contain the correct page, corresponding to candidatePageNum

    int keysRead = itemsRead / 2;

    int mid = binSearch(0, keysRead - 1, [&](int m) {
        return (currKey < pageBuf[m * 2]) ? -1 : (currKey > pageBuf[m * 2]) ? 1 : 0;
    });

    // At this point, mid is either equal to the index of currKey itself,
    // or the next smallest key after currKey (if currKey wasn't found)

    uint64_t midKey;

    while (!keysToFind.empty()) {
        currKey = keysToFind.back();
        midKey = pageBuf[mid * 2];

        while (midKey < currKey) {
            mid++;

            // If mid is out of bounds, read next page and set mid to 0
            if (mid >= keysRead) {
                candidatePageNum++;
                bytesRead =
                    bufferPool->comboRead(SST_PATH(sstNum), sstNum, PAGE_SIZE * candidatePageNum, pageBuf, PAGE_SIZE);
                // bytesRead = pread(fd, pageBuf, PAGE_SIZE, PAGE_SIZE * candidatePageNum);
                itemsRead = bytesRead / sizeof(uint64_t);
                keysRead = itemsRead / 2;

                mid = 0;
            }

            midKey = pageBuf[mid * 2];
        }

        // Now, midKey >= currKey
        if (midKey == currKey) {
            foundPairs.push_back(std::make_tuple(midKey, pageBuf[mid * 2 + 1]));
        } else {
            keysNotFound.push_back(currKey);
        }

        keysToFind.pop_back();
    }

    return std::make_tuple(foundPairs, keysNotFound);
}

}
