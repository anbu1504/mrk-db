#include "../include/hashmap.hpp"

HPage::HPage(std::string pageID, bool dirtyBit, bool refBit, uint64_t* cachedPage)
  : pageID(pageID), dirtyBit(dirtyBit), refBit(refBit), cachedPage(cachedPage) {}

HashMap::HashMap() : cachedPages(cacheSize, HPage()) {}
uint64_t* HashMap::Get(std::string pageID) { return nullptr; }
void HashMap::Put(std::string pageID, PageBuffer pageBuf) { return; }
void HashMap::DeleteAllWithPrefix(std::string fileName) { return; }
void HashMap::DeleteAll() { return; }
