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
    BufferPool(size_t initial, size_t maximal); // constructor with threshold
    ~BufferPool();                              // destructor to free memory
    std::optional<uint64_t *> searchPage(int sstNum, int pageOffset);
    int addPage(uint64_t *buffer);
    int evict();

private:
    HashMap hashMap;
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

    std::vector<std::tuple<int, Node *, size_t>> directory;

    HashMap(size_t initial); // constructor with initial size
    ~HashMap();              // destructor to free memory

    int insert(std::string pageName, uint64_t *page); // 0 on success 1 on fail
    std::tuple<int, uint64_t *> search(std::string pageName);
    int remove(std::string pageName); // 0 on success 1 on fail
    int extend();                     // 0 on success 1 on fail

private:
    int hashFunction(std::string key);
};