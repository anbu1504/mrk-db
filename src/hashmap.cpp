#include "../include/hashmap.hpp"

#include "../external/xxhash64.h"

#include <unistd.h>
#include <sys/fcntl.h>

#define SST_PATH_STR(x) ((dbName + "/" + x + ".sst").c_str())

HPage::HPage(std::string pageID, bool dirtyBit, bool refBit, uint64_t probeSeqLen, uint64_t* cachedPage)
  : pageID(pageID), dirtyBit(dirtyBit), refBit(refBit), probeSeqLen(probeSeqLen), cachedPage(cachedPage) {}

void HPage::reset() {
    std::string pageID = "";
    bool dirtyBit = false;
    bool refBit = true;
    uint64_t probeSeqLen = 0;
    if (cachedPage) {
        free(cachedPage);
        uint64_t* cachedPage = nullptr;
    }
}

HashMap::HashMap(std::string dbName) : dbName(dbName), numCachedPages(0), clockHandle(0), cacheVec(cacheSize, HPage()) {}

HashMap::~HashMap() {
    for (int hPageNum = 0; hPageNum < cacheSize; hPageNum++) {
        cacheVec[hPageNum].reset();
    }
}

HPage* HashMap::getHPage(std::string pageID) {
    uint64_t cacheIdx = XXHash64::hash(&pageID, sizeof(uint64_t), 0) % cacheSize;
    uint64_t currProbeSeqLen = 0;
    while (cacheVec[cacheIdx].cachedPage && !(cacheVec[cacheIdx].pageID == pageID) && !(currProbeSeqLen > cacheVec[cacheIdx].probeSeqLen)) {
        cacheIdx = (cacheIdx + 1) % cacheSize;
        currProbeSeqLen++;
    }

    if (cacheVec[cacheIdx].pageID == pageID) {
        return &cacheVec[cacheIdx];
    }

    return nullptr;
}

uint64_t* HashMap::get(std::string pageID) {
    HPage* getAttempt = getHPage(pageID);
    if (getAttempt) {
        getAttempt->refBit = true;
        return getAttempt->cachedPage;
    }
    return nullptr;
}

void HashMap::put(std::string pageID, PageBuffer pageBuf) {
    HPage* getAttempt = getHPage(pageID);
    if (getAttempt) { // If the page already exists, update it
        memcpy(getAttempt->cachedPage, pageBuf, PAGE_SIZE);
        getAttempt->dirtyBit = true;
        getAttempt->refBit = true;
    }

    if (numCachedPages == cacheSize) {
        runClock();
    }

    uint64_t* newPage = (uint64_t*) aligned_alloc(PAGE_SIZE, PAGE_SIZE);
    memcpy(newPage, pageBuf, PAGE_SIZE);

    HPage tempHPage = HPage(pageID, true, true, 0, newPage);

    uint64_t cacheIdx = XXHash64::hash(&pageID, sizeof(uint64_t), 0) % cacheSize;
    while (cacheVec[cacheIdx].cachedPage) { // While we keep bumping into existing entries

        if (tempHPage.probeSeqLen > cacheVec[cacheIdx].probeSeqLen) {
            std::swap(cacheVec[cacheIdx], tempHPage);
        }

        cacheIdx = (cacheIdx + 1) % cacheSize;
        tempHPage.probeSeqLen++;
        break;
    }

    // Now, cacheIdx should be the idx of a free node
    std::swap(cacheVec[cacheIdx], tempHPage);
    numCachedPages++;
}

void HashMap::deleteAllWithPrefix(std::string fileName) { 
    for (int hPageNum = 0; hPageNum < cacheSize; hPageNum++) {
        // Note that string.compare() returns 0 if string starts with fileName
        if (!(cacheVec[hPageNum].pageID.compare(0, fileName.length(), fileName))) {
            cacheVec[hPageNum].reset();
        }
    }
}

void HashMap::evictAll() { 
    for (int hPageNum = 0; hPageNum < cacheSize; hPageNum++) {
        if (cacheVec[hPageNum].cachedPage && cacheVec[hPageNum].dirtyBit) {
            evict(&cacheVec[hPageNum]);
        }
    }
}

void HashMap::runClock() {
    while (numCachedPages == cacheSize) {
        if (cacheVec[clockHandle].refBit) {
            cacheVec[clockHandle].refBit = false;
        } else {
            evict(&cacheVec[clockHandle]);
            numCachedPages--;
        }
        clockHandle++;
    }
    return;
}

void HashMap::evict(HPage* victim) {
    uint64_t underscoreIdx = victim->pageID.find("_");
    std::string sstNum = victim->pageID.substr(0, underscoreIdx);
    uint64_t pageNum = std::stoull(victim->pageID.substr(underscoreIdx + 1));

    int fd = open(SST_PATH_STR(sstNum), O_RDWR | O_CREAT, 0644);
    pwrite(fd, victim->cachedPage, PAGE_SIZE, pageNum * PAGE_SIZE);
    close(fd);

    victim->reset();
}
