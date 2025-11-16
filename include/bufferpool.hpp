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

class BufferPool
{
public:
    BufferPool(size_t initialDirSize, size_t maxDirSize, size_t maxPages); // constructor
    ~BufferPool(); // destructor to free memory
    size_t initialDirSize; // Initial number of buckets in hash map
    size_t maxDirSize; // Maximum number of buckets in hash map
    size_t maxPages; // Maximum number of pages in buffer pool
    std::optional<std::tuple<int, uint64_t *>> searchPage(int sstNum, int pageOffset);
    int addPage(uint64_t *buffer);
    int evict();

private:
    HashMap hashMap;
    std::vector<std::string> storedPages;
    std::optional<std::string> get_next_eviction();
};

class HashMap
{
public:
    int bufferOverflowThreshold;
    int numBitsUsed; // used for directory

    struct Node
    {
        std::string pageName;
        uint64_t *page;
        Node *next;
        bool accessBit;

        Node(std::string name, uint64_t *p)
            : pageName(name),
              page(p),
              next(nullptr)
        {
        }
    };

    struct DirEntry {
        Node * first;
        Node * tail;
        size_t chainSize;
        size_t numHashedDigits; // number of digits used to differentiate this bucket
        size_t hashedIndex;

        DirEntry(size_t numDigits, size_t index)
            : first(nullptr),
              tail(nullptr),
              chainSize(0),
              numHashedDigits(numDigits),
              hashedIndex(index)
        {
        }
    };

    std::vector<DirEntry *> directory;

    HashMap(size_t initial); // constructor with initial size
    ~HashMap();              // destructor to free memory

    int insert(std::string pageName, uint64_t *page); // 0 on success 1 on fail
    std::optional<std::tuple<int, uint64_t *>> search(std::string pageName);
    int remove(std::string pageName); // 0 on success 1 on fail
    int extend();                     // 0 on success 1 on fail
    int rehashBuckets();

private:
    uint64_t hashFunction(std::string key);
    void insertNodeToBucket(Node *node, DirEntry *dirEntry);
    void rehashBucket(DirEntry *dirEntry);
};