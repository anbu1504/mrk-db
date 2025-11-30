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
    HPage(std::string pageID = "", bool dirtyBit = false, bool refBit = true, uint64_t probeSeqLen = 0, uint64_t* cachedPage = nullptr);
    void reset();
};

class HashMap {
   private:
    std::string dbName;
    uint64_t numCachedPages;
    uint64_t clockHandle;
    std::vector<HPage> cacheVec;
    void runClock();
    void evict(HPage* victim);
    HPage* HashMap::getHPage(std::string pageID);

   public:
   HashMap(std::string dbName);
   ~HashMap();
    uint64_t* get(std::string pageID);
    void put(std::string pageID, PageBuffer pageBuf);
    void deleteAllWithPrefix(std::string fileName);
    void evictAll();
};
