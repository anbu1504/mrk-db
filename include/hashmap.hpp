#pragma once

#include "constants.hpp"
#include "globals.hpp"

class HPage {
   public:
    std::string pageID;
    bool dirtyBit; // Used during eviction
    bool refBit; // Used by the clock algorithm
    uint64_t* cachedPage;
    HPage(std::string pageID = "", bool dirtyBit = false, bool refBit = true, uint64_t* cachedPage = nullptr);
};

class HashMap {
   private:
    uint64_t numCachedPages;
    uint64_t clockHandle;
    std::vector<HPage> cachedPages;

   public:
   HashMap();
    uint64_t* Get(std::string pageID);
    void Put(std::string pageID, PageBuffer pageBuf);
    void DeleteAllWithPrefix(std::string fileName);
    void DeleteAll();
};
