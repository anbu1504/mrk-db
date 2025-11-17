#pragma once

#include <iostream>
#include <cstdint>
#include <algorithm>
#include <fstream>
#include <string>
#include <vector>
#include <tuple>
#include <optional>
#define PAGE_SIZE 4096
#define BUCKET_OVERFLOW_THRESHOLD 4

class HashMap
{
    friend class HashMapTester;

public:
    int bucketOverflowThreshold;
    int numBitsUsed; // used for directory

    struct Node
    {
        std::string pageName;
        uint64_t *page;
        Node *next;
        bool accessBit;
        size_t pageSize;

        Node(std::string name, uint64_t *p, size_t size)
            : pageName(name),
              page(p),
              next(nullptr),
              accessBit(true),
              pageSize(size)
        {
        }
    };

    struct DirEntry
    {
        Node *first;
        Node *tail;
        size_t chainSize;
        int numHashedDigits; // number of digits used to differentiate this bucket
        size_t hashedIndex;

        DirEntry()
            : first(nullptr),
              tail(nullptr),
              chainSize(0),
              numHashedDigits(0),
              hashedIndex(0)
        {
        }
    };

    std::vector<DirEntry *> directory;
    size_t maxDirSize;

    HashMap(size_t initial, size_t maxDirSize); // constructor with initial size
    ~HashMap();                                 // destructor to free memory

    int insert(std::string pageName, uint64_t *page, size_t pageSize); // 0 on success 1 on fail
    std::optional<Node *> search(std::string pageName);
    std::optional<Node *> remove(std::string pageName); // 0 on success 1 on fail
    int extendDir();                                    // 0 on success 1 on fail
    int rehashBuckets();

private:
    uint64_t hashFunction(std::string key);
    void insertNodeToBucket(Node *node, DirEntry *dirEntry);
    void rehashBucket(DirEntry *dirEntry);
};

class BufferPool
{
public:
    BufferPool(size_t initialDirSize, size_t maxDirSize, size_t maxPages); // constructor
    ~BufferPool();                                                         // destructor to free memory
    size_t initialDirSize;                                                 // Initial number of buckets in hash map
    size_t maxDirSize;                                                     // Maximum number of buckets in hash map
    size_t maxPages;
    size_t numPages; // Maximum number of pages in buffer pool
    std::optional<std::tuple<int, uint64_t *>> searchPage(int sstNum, int pageOffset);
    int addPage(uint64_t *buffer);

private:
    HashMap hashMap;
    std::vector<std::string> storedPages;
    std::optional<std::string> get_next_eviction();
    int evict();
};