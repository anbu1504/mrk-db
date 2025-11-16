#include "bufferpool.hpp";
#include "xxhash64.h";
#include <cmath>;

/**
 * @brief Constructor for the BufferPool class
 */

BufferPool::BufferPool(size_t initial, size_t maximal)
    : hashMap(initial) {};

/**
 * @brief Destructor for the BufferPool class
 */
BufferPool::~BufferPool()
{
}

/**
 * @brief Constructor for the HashMap class
 */

HashMap::HashMap(size_t initial)
{
    numBitsUsed = ceil(log2(initial)); // for global depth of directory
    bufferOverflowThreshold = 3;

    // Initializing buckets

    for (size_t i = 0; i < initial; i++)
    {
        directory[i] = std::make_tuple(
            i,
            nullptr,
            0);
    };
}

/**
 * @brief Destructor for the HashMap class
 */

HashMap::~HashMap()
{
    for (auto &[depth, head, count] : directory)
    {
        Node *curr = head;
        while (curr)
        {
            Node *tempNext = curr->next;
            delete curr;
            curr = tempNext;
        }
    }
}

uint64_t HashMap::hashFunction(std::string key)
{
    // CITE THE GITHUB LINK: https://github.com/stbrumme/xxhash/blob/master/xxhash64.h
    uint64_t h = XXHash64::hash(key.data(), key.size(), 0); // 0 is the seed
    return h;
}

int HashMap::insert(std::string pageName, uint64_t *page)
{
    uint64_t hashedPageName = hashFunction(pageName);
    uint64_t mask = 1ULL << hashedPageName - 1;

    uint64_t maskedHashPage = hashedPageName & mask;

    DirEntry *dirEntry = directory[maskedHashPage];
    Node *insertNode = new Node(pageName, page);

    if (!dirEntry->first)
    {
        dirEntry->first = insertNode;
        dirEntry->tail = insertNode;
    }

    else
    {
        dirEntry->tail->next = insertNode;
        dirEntry->tail = insertNode;
    }
    dirEntry->chainSize++;

    if (dirEntry->chainSize >= bufferOverflowThreshold)
    {
        if (dirEntry->numHashedDigits < numBitsUsed)
        {
            // rehash buckets
        }
        else
        {
            // extend directory + rehash buckets :)
        }
    }
}
