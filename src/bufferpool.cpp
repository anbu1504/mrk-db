#include "../include/bufferpool.hpp"

#include "../external/xxhash64.h"

#include <cstring>
#include <unistd.h>
#include <sys/fcntl.h>
#include <assert.h>

BufferPool::BufferPool(std::string dbName) : dbName(dbName), numCachedPages(0), clockHandle(0), hashTable(cacheSize, HPage()) {}

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

        // debugging
        // int fd = open(createSSTPath(sstNum).c_str(), O_RDONLY);
        // pread(fd, pageBuf, PAGE_SIZE, pageNum * PAGE_SIZE);
        // close(fd);
    } else {
        int fd = open(createSSTPath(sstNum).c_str(), O_RDONLY);
        pread(fd, pageBuf, PAGE_SIZE, pageNum * PAGE_SIZE);
        close(fd);
        cachePut(createPageID(sstNum, pageNum), pageBuf, false);
    }
}

void BufferPool::bwrite(uint64_t sstNum, uint64_t pageNum, PageBuffer pageBuf) {
    cachePut(createPageID(sstNum, pageNum), pageBuf, true);
    int fd = open(createSSTPath(sstNum).c_str(), O_RDWR | O_CREAT, 0644);
    pwrite(fd, pageBuf, PAGE_SIZE, pageNum * PAGE_SIZE);
    close(fd);
}

void BufferPool::bdelete(uint64_t sstNum) {
    // Check the filesystem for the file (if it exists on disk)
    std::remove(createSSTPath(sstNum).c_str());

    // Scan the hashtable for entries w/ matching prefixes, and delete em
    std::string sstString = std::to_string(sstNum);
    for (uint64_t hPageNum = 0; hPageNum < cacheSize; hPageNum++) {
        // Note that string.compare() returns 0 if string starts with fileName
        if (!(hashTable[hPageNum].pageID.compare(0, sstString.length(), sstString))) {
            hashTable[hPageNum].reset();
        }
    }
}

void BufferPool::evictAllPages() {
    for (uint64_t hPageNum = 0; hPageNum < cacheSize; hPageNum++) {
        if (hashTable[hPageNum].cachedPage && hashTable[hPageNum].dirtyBit) {
            evict(&hashTable[hPageNum]);
        }
    }
}

HPage* BufferPool::cacheGet(std::string pageID) {
    uint64_t cacheIdx = XXHash64::hash(pageID.data(), pageID.size(), 0) % cacheSize;
    uint64_t currProbeSeqLen = 0;
    while (hashTable[cacheIdx].cachedPage && !(hashTable[cacheIdx].pageID == pageID) && !(currProbeSeqLen > hashTable[cacheIdx].probeSeqLen)) {
        cacheIdx = (cacheIdx + 1) % cacheSize;
        currProbeSeqLen++;
    }

    if (hashTable[cacheIdx].pageID == pageID) {
        hashTable[cacheIdx].refBit = true;
        return &hashTable[cacheIdx];
    }

    return nullptr;
}

// ========== PRIVATE METHODS ==========

void BufferPool::cachePut(std::string pageID, PageBuffer pageBuf, bool dirty) {
    HPage* getAttempt = cacheGet(pageID);
    if (getAttempt) { // If the page already exists, update it
        memcpy(getAttempt->cachedPage, pageBuf, PAGE_SIZE);
        getAttempt->dirtyBit = dirty; // refBit should also be true
        return;
    }

    runClockIfFull();

    uint64_t* newPage = (uint64_t*) aligned_alloc(PAGE_SIZE, PAGE_SIZE);
    memcpy(newPage, pageBuf, PAGE_SIZE);

    HPage tempHPage = HPage(pageID, dirty, true, 0, newPage);

    uint64_t cacheIdx = XXHash64::hash(pageID.data(), pageID.size(), 0) % cacheSize;
    while (hashTable[cacheIdx].cachedPage) { // While we keep bumping into existing entries

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

void BufferPool::evict(HPage* victim) {
    uint64_t underscoreIdx = victim->pageID.find("_");
    uint64_t sstNum = std::stoull(victim->pageID.substr(0, underscoreIdx));
    uint64_t pageNum = std::stoull(victim->pageID.substr(underscoreIdx + 1));

    int fd = open(createSSTPath(sstNum).c_str(), O_RDWR | O_CREAT, 0644);
    pwrite(fd, victim->cachedPage, PAGE_SIZE, pageNum * PAGE_SIZE);
    close(fd);

    victim->reset();
}

void BufferPool::runClockIfFull() {
    while (numCachedPages == cacheSize) {
        if (hashTable[clockHandle].refBit) {
            hashTable[clockHandle].refBit = false;
        } else {
            evict(&hashTable[clockHandle]);
            numCachedPages--;
        }
        clockHandle = (clockHandle + 1) % cacheSize;
    }
    return;
}
