#pragma once

#include "constants.hpp"
#include "globals.hpp"

class HPage {
   public:
    std::string pageID;
    bool dirtyBit; // Used during eviction
    bool refBit; // Used by the clock algorithm
    uint64_t probeSeqLen;
    uint64_t* cachedPage;

    HPage(std::string pageID = "", bool dirtyBit = false, bool refBit = true,
          uint64_t probeSeqLen = 0, uint64_t* cachedPage = nullptr)
      : pageID(pageID), dirtyBit(dirtyBit), refBit(refBit),
        probeSeqLen(probeSeqLen), cachedPage(cachedPage) {}
    
    void reset() {
        pageID.clear();
        dirtyBit = false;
        refBit = true;
        probeSeqLen = 0;
        if (cachedPage) {
            free(cachedPage);
            cachedPage = nullptr;
        }
    }
};

class BufferPool {
   private:
    std::string dbName;
    uint64_t numCachedPages;
    uint64_t clockHandle;
    std::vector<HPage> hashTable; // An open-addressing hash table, using Robin Hood hashing

    HPage* cacheGet(std::string pageID);
    void cachePut(std::string pageID, PageBuffer pageBuf, bool dirty);
    bool cacheDel(uint64_t pageIdx);
    bool evict(uint64_t victimIdx) ;
    void runClockIfFull();
    // void printHashTable();

    std::string createSSTPath(uint64_t sstNum);
    std::string createPageID(uint64_t sstNum, uint64_t pageNum);

   public:
    BufferPool(std::string dbNameVal);  // constructor
    ~BufferPool();                      // destructor to free memory

    void bread(uint64_t sstNum, uint64_t pageNum, PageBuffer pageBuf, bool bypassCache = false);  // return number of bytes read
    void bwrite(uint64_t sstNum, uint64_t pageNum, PageBuffer pageBuf);  // return number of bytes written
    void bdelete(uint64_t sstNum); // Evicts all pages in bufferpool of given file and deletes the file
    void evictAllPages();                // evicts all pages and writes dirty pages to storage
};
