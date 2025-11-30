#include "../include/bufferpool.hpp"

#include "../external/xxhash64.h"

#include <cstring>
#include <unistd.h>
#include <sys/fcntl.h>

#define PAGE_ID(s, p) (std::to_string(s) + "_" + std::to_string(p))
#define SST_PATH(x) ((dbName + "/" + std::to_string(x) + ".sst").c_str())

HPage::HPage(std::string pageID, bool dirtyBit, bool refBit, uint64_t probeSeqLen, uint64_t* cachedPage)
  : pageID(pageID), dirtyBit(dirtyBit), refBit(refBit), probeSeqLen(probeSeqLen), cachedPage(cachedPage) {}

void HPage::reset() {
    pageID = "";
    dirtyBit = false;
    refBit = true;
    probeSeqLen = 0;
    if (cachedPage) {
        free(cachedPage);
        cachedPage = nullptr;
    }
};

BufferPool::BufferPool(std::string dbName) : dbName(dbName), numCachedPages(0), clockHandle(0), hashTable(cacheSize, HPage()) {}

BufferPool::~BufferPool() {
    for (uint64_t hPageNum = 0; hPageNum < cacheSize; hPageNum++) {
        hashTable[hPageNum].reset();
    }
}

// ========== PUBLIC METHODS ==========

void BufferPool::bread(uint64_t sstNum, uint64_t pageNum, PageBuffer pageBuf, bool bypassCache) {
    HPage* cachedHPage = cacheGet(PAGE_ID(sstNum, pageNum));
    if (!bypassCache && cachedHPage) {
        memcpy(pageBuf, cachedHPage->cachedPage, PAGE_SIZE);
    } else {
        int fd = open(SST_PATH(sstNum), O_RDONLY);
        pread(fd, pageBuf, PAGE_SIZE, pageNum * PAGE_SIZE);
        close(fd);
        cachePut(PAGE_ID(sstNum, pageNum), pageBuf, false);
    }
}

void BufferPool::bwrite(uint64_t sstNum, uint64_t pageNum, PageBuffer pageBuf) {
    cachePut(PAGE_ID(sstNum, pageNum), pageBuf, true);
}

void BufferPool::bdelete(uint64_t sstNum) {
    // Check the filesystem for the file (if it exists on disk)
    std::remove(SST_PATH(sstNum));

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
    uint64_t cacheIdx = XXHash64::hash(&pageID, sizeof(uint64_t), 0) % cacheSize;
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

    uint64_t cacheIdx = XXHash64::hash(&pageID, sizeof(uint64_t), 0) % cacheSize;
    while (hashTable[cacheIdx].cachedPage) { // While we keep bumping into existing entries

        if (tempHPage.probeSeqLen > hashTable[cacheIdx].probeSeqLen) {
            std::swap(hashTable[cacheIdx], tempHPage);
        }

        cacheIdx = (cacheIdx + 1) % cacheSize;
        tempHPage.probeSeqLen++;
        break;
    }

    // Now, cacheIdx should be the idx of a free node
    std::swap(hashTable[cacheIdx], tempHPage);
    numCachedPages++;

}

void BufferPool::evict(HPage* victim) {
    uint64_t underscoreIdx = victim->pageID.find("_");
    uint64_t sstNum = std::stoull(victim->pageID.substr(0, underscoreIdx));
    uint64_t pageNum = std::stoull(victim->pageID.substr(underscoreIdx + 1));

    int fd = open(SST_PATH(sstNum), O_RDWR | O_CREAT, 0644);
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
        clockHandle++;
    }
    return;
}


    