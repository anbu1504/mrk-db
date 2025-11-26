#pragma once

#include "constants.hpp"

class HashMap {
    friend class HashMapTester;

   public:
    int bucketOverflowThreshold;
    int numBitsUsed;  // used for directory

    struct Node {
        std::string pageName;
        uint64_t* page;
        Node* next;
        bool accessBit;
        size_t pageSize;

        Node(std::string name, uint64_t* p, size_t size)
            : pageName(name), page(p), next(nullptr), accessBit(true), pageSize(size) {}
    };

    struct DirEntry {
        Node* first;
        Node* tail;
        size_t chainSize;
        int numHashedDigits;  // number of digits used to differentiate this bucket
        size_t hashedIndex;

        DirEntry() : first(nullptr), tail(nullptr), chainSize(0), numHashedDigits(0), hashedIndex(0) {}
    };

    std::vector<DirEntry*> directory;
    size_t maxDirSize;

    HashMap(size_t initial, size_t maxDirSize);  // constructor with initial size
    ~HashMap();                                  // destructor to free memory

    int insert(std::string pageName, uint64_t* page, size_t pageSize);  // 0 on success 1 on fail
    std::optional<Node*> search(std::string pageName);
    std::optional<Node*> remove(std::string pageName);  // 0 on success 1 on fail
    int extendDir();                                    // 0 on success 1 on fail
    int rehashBuckets();

   private:
    uint64_t hashFunction(std::string key);
    void insertNodeToBucket(Node* node, DirEntry* dirEntry);
    void rehashBucket(DirEntry* dirEntry);
};

class BufferPool {
   private:
    HashMap* hashMap;
    std::vector<std::string> clockVector;
    uint64_t* evict();
    uint64_t clockHandle;
    size_t initialDirSize;                                                  // Initial number of buckets in hash map
    size_t maxDirSize;                                                      // Maximum number of buckets in hash map
    size_t maxPages;
    size_t numPages;  // Maximum number of pages in buffer pool
    std::string dbName;
    std::optional<std::tuple<uint64_t, uint64_t*>> searchPage(uint64_t sstNum, uint64_t pageOffset);
    uint64_t* addPage(uint64_t sstNum, uint64_t pageOffset, uint64_t* buffer, size_t pageSize);

   public:
    BufferPool(size_t initialDirSize, size_t maxDirSize, size_t maxPages, std::string dbName);  // constructor
    ~BufferPool();                                                          // destructor to free memory

    ssize_t bread(uint64_t sstNum, uint64_t pageNum, PageBuffer buffer);  // return number of bytes read
    ssize_t bwrite(uint64_t sstNum, uint64_t pageNum, PageBuffer buffer);  // return number of bytes written
};
