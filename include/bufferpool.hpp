#pragma once

#include "constants.hpp"
#include "hashmap.hpp"

class BufferPool {
   private:
   std::string dbName;
    HashMap* hashMap;

   public:
    BufferPool(std::string dbNameVal);  // constructor
    ~BufferPool();                      // destructor to free memory

    void bread(uint64_t sstNum, uint64_t pageNum, PageBuffer pageBuf, bool bypassCache = false);  // return number of bytes read
    // ssize_t bread(std::string filename, uint64_t pageNum, PageBuffer buffer, bool bypassCache = false);
    void bwrite(uint64_t sstNum, uint64_t pageNum, PageBuffer pageBuf);  // return number of bytes written
    // ssize_t bwrite(std::string filename, uint64_t pageNum, PageBuffer buffer);
    void bdelete(uint64_t sstNum);
    // void bdelete(std::string filename);  // Evicts all pages in bufferpool of given file and deletes the file
    void evictAllPages();                // evicts all pages and writes dirty pages to storage
};
