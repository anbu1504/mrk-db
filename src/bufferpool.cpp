#include "../include/bufferpool.hpp"

#include <assert.h>
#include <sys/fcntl.h>
#include <unistd.h>

#include <cstring>
#include <iomanip>

#include "../external/xxhash64.h"

BufferPool::BufferPool(std::string dbName)
    : dbName(dbName), numCachedPages(0), clockHandle(0), hashTable(cacheSize, HPage()) {}

BufferPool::~BufferPool() {
    for (uint64_t hPageNum = 0; hPageNum < cacheSize; hPageNum++) {
        hashTable[hPageNum].reset();
    }
}

std::string BufferPool::createSSTPath(uint64_t x) { return dbName + "/" + std::to_string(x) + ".sst"; }
std::string BufferPool::createPageID(uint64_t s, uint64_t p) { return std::to_string(s) + "_" + std::to_string(p); }

// ========== PUBLIC METHODS ==========

void BufferPool::bread(uint64_t sstNum, uint64_t pageNum, PageBuffer pageBuf, bool bypassCache) {
    HPage* cachedHPage = cacheGet(createPageID(sstNum, pageNum));
    if (!bypassCache && cachedHPage) {
        memcpy(pageBuf, cachedHPage->cachedPage, PAGE_SIZE);
    } else {
        int fd = open(createSSTPath(sstNum).c_str(), O_RDONLY | O_DIRECT);
        pread(fd, pageBuf, PAGE_SIZE, pageNum * PAGE_SIZE);
        close(fd);
        cachePut(createPageID(sstNum, pageNum), pageBuf, false);
    }
}

void BufferPool::bwrite(uint64_t sstNum, uint64_t pageNum, PageBuffer pageBuf) {
    cachePut(createPageID(sstNum, pageNum), pageBuf, true);
}

void BufferPool::bdelete(uint64_t sstNum) {
    // Check the filesystem for the file (if it exists on disk)
    std::remove(createSSTPath(sstNum).c_str());

    // Scan the hashtable for entries w/ matching prefixes, and delete em
    // std::string sstString = std::to_string(sstNum);

    uint64_t hPageNum = 0;

    for (uint64_t visitedPages = 0; visitedPages < cacheSize; visitedPages++) {
        if (!hashTable[hPageNum].cachedPage) {
            hPageNum++;
            continue;
        }

        uint64_t underscoreIdx = hashTable[hPageNum].pageID.find("_");
        uint64_t hPageSSTNum = std::stoull(hashTable[hPageNum].pageID.substr(0, underscoreIdx));
        if (hPageSSTNum == sstNum) {
            // If we found a matching page, delete that page. If a backshift occurred, then
            // keep hPageNum the same. Otherwise, move onto the next page.
            hPageNum += !cacheDel(hPageNum);
        } else {
            hPageNum++;
        }
    }
}

void BufferPool::evictAllPages() {
    uint64_t hPageNum = 0;
    for (uint64_t visitedPages = 0; visitedPages < cacheSize; visitedPages++) {
        if (hashTable[hPageNum].cachedPage) {
            // If we found an existing page, evict that page. If a backshift occurred, then
            // keep hPageNum the same. Otherwise, move onto the next page.
            hPageNum += !evict(hPageNum);
        } else {
            hPageNum++;
        }
    }
}

// ========== PRIVATE METHODS ==========

HPage* BufferPool::cacheGet(std::string pageID) {
    uint64_t cacheIdx = XXHash64::hash(pageID.data(), pageID.size(), 0) % cacheSize;
    uint64_t currProbeSeqLen = 0;
    while (hashTable[cacheIdx].cachedPage && !(hashTable[cacheIdx].pageID == pageID) &&
           !(currProbeSeqLen > hashTable[cacheIdx].probeSeqLen)) {
        cacheIdx = (cacheIdx + 1) % cacheSize;
        currProbeSeqLen++;
    }

    if (hashTable[cacheIdx].pageID == pageID) {
        hashTable[cacheIdx].refBit = true;
        return &hashTable[cacheIdx];
    }

    return nullptr;
}

void BufferPool::cachePut(std::string pageID, PageBuffer pageBuf, bool dirty) {
    HPage* getAttempt = cacheGet(pageID);
    if (getAttempt) {  // If the page already exists, update it
        memcpy(getAttempt->cachedPage, pageBuf, PAGE_SIZE);
        getAttempt->dirtyBit = dirty;  // refBit should also be true
        return;
    }

    runClockIfFull();

    uint64_t* newPage = (uint64_t*)aligned_alloc(PAGE_SIZE, PAGE_SIZE);
    memcpy(newPage, pageBuf, PAGE_SIZE);

    HPage tempHPage = HPage(pageID, dirty, true, 0, newPage);

    uint64_t cacheIdx = XXHash64::hash(pageID.data(), pageID.size(), 0) % cacheSize;
    while (hashTable[cacheIdx].cachedPage) {  // While we keep bumping into existing entries

        if (tempHPage.probeSeqLen > hashTable[cacheIdx].probeSeqLen) {
            std::swap(hashTable[cacheIdx], tempHPage);
        }

        cacheIdx = (cacheIdx + 1) % cacheSize;
        tempHPage.probeSeqLen++;
    }

    // Now, cacheIdx should be the idx of a free node
    std::swap(hashTable[cacheIdx], tempHPage);
    numCachedPages++;

    assert(!tempHPage.cachedPage);
}

// Performs a backshift delete
bool BufferPool::cacheDel(uint64_t pageIdx) {
    bool shifted = false;
    hashTable[pageIdx].reset();
    // if theres a page at the next idx who is displaced, move it back
    while (hashTable[(pageIdx + 1) % cacheSize].cachedPage && hashTable[(pageIdx + 1) % cacheSize].probeSeqLen) {
        shifted = true;
        hashTable[(pageIdx + 1) % cacheSize].probeSeqLen--;
        std::swap(hashTable[pageIdx], hashTable[(pageIdx + 1) % cacheSize]);
        pageIdx = (pageIdx + 1) % cacheSize;
    }

    numCachedPages--;
    return shifted;
}

bool BufferPool::evict(uint64_t victimIdx) {
    HPage victim = hashTable[victimIdx];
    if (victim.dirtyBit) {
        uint64_t underscoreIdx = victim.pageID.find("_");
        uint64_t sstNum = std::stoull(victim.pageID.substr(0, underscoreIdx));
        uint64_t pageNum = std::stoull(victim.pageID.substr(underscoreIdx + 1));

        int fd = open(createSSTPath(sstNum).c_str(), O_RDWR | O_CREAT | O_DIRECT, 0644);
        pwrite(fd, victim.cachedPage, PAGE_SIZE, pageNum * PAGE_SIZE);
        close(fd);
    }

    return cacheDel(victimIdx);
}

void BufferPool::runClockIfFull() {
    bool shifted = false;
    while (numCachedPages == cacheSize) {
        if (hashTable[clockHandle].refBit) {
            hashTable[clockHandle].refBit = false;
        } else {
            shifted = evict(clockHandle);
        }
        // Need to think about this logic depending on whether backshift delete moves stuff or not.
        // Ok so, if we shifted things back, then no need to move the clock handle!!
        clockHandle = (clockHandle + !shifted) % cacheSize;
    }
    return;
}

// void BufferPool::printHashTable() {
//     std::cout << "=== BufferPool Hash Table ===\n";
//     std::cout << "idx\tpageID\t\tdirty ref probe cached?\n";
//     for (uint64_t i = 0; i < cacheSize; i++) {
//         const HPage& h = hashTable[i];
//         if (!h.cachedPage) continue;
//         std::cout << i << "\t"
//                   << h.pageID << "\t"
//                   << (h.dirtyBit ? "D" : "-") << "     "
//                   << (h.refBit ? "R" : "-") << "   "
//                   << std::setw(2) << h.probeSeqLen << "   "
//                   << "0x" << std::hex << (uintptr_t)h.cachedPage << std::dec
//                   << "\n";
//     }
//     std::cout << "numCachedPages=" << numCachedPages
//               << " clockHandle=" << clockHandle << "\n";
// }
