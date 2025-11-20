#include "bufferpool.hpp"
#include "../external/xxhash64.h"
#include <unordered_set>
#include <cmath>

#define PRINT(x) (std::cout << x << std::endl)

/**
 * @brief Constructor for the BufferPool class
 */

BufferPool::BufferPool(size_t initialAmount, size_t maximalAmount, size_t maxPagesAmount)
    : initialDirSize(initialAmount),
      maxDirSize(maximalAmount),
      maxPages(maxPagesAmount),
      numPages(0),
      hashMap(new HashMap(initialAmount, maximalAmount)),
      clockHandle(0)
{
    clockVector.reserve(maxPagesAmount);
};

/**
 * @brief Destructor for the BufferPool class
 */
BufferPool::~BufferPool()
{
}

std::optional<std::tuple<int, uint64_t *>> BufferPool::searchPage(int sstNum, int pageOffset)
{
    std::string pageName = std::to_string(sstNum) + "_" + std::to_string(pageOffset);
    std::optional<HashMap::Node *> searchResult = hashMap->search(pageName);

    if (searchResult.has_value())
    {
        HashMap::Node *node = searchResult.value();
        node->accessBit = true;
        std::tuple<int, uint64_t *> returnValue = std::make_tuple(node->pageSize, node->page);
        return returnValue;
    }
    else
    {
        return std::nullopt;
    }
}

// Maybe don't need the error code
// Return Value: Tuple of error code and buffer of evicted page (or nullptr)
std::tuple<int, uint64_t *> BufferPool::addPage(int sstNum, int pageOffset, uint64_t *buffer, size_t pageSize)
{
    // Assert that this page is not in bufferpool already?
    std::string pageName = std::to_string(sstNum) + "_" + std::to_string(pageOffset);
    uint64_t *evictedBuffer = nullptr;

    if (numPages == maxPages){
        evictedBuffer = evict(); // Elaborate on after implementing evict
        numPages = numPages - 1;
    }

    PRINT("top");
    int insertResult = hashMap->insert(pageName, buffer, pageSize);
    PRINT("bottom");

    // Evict until successful insert option: (Prolly not needed if we disable the chain limit when directory is max)
    // while (insertResult != 0){
    //     evictedIndex evict();
    //     insertResult = hashMap.insert(pageName, buffer, pageSize);
    // }

    // Fail after one insert option: (Also prolly not needed if we disable the chain limit when directory is max)
    if (insertResult != 0){
        return std::make_tuple(1, evictedBuffer);
    }

    if (evictedBuffer){
        clockVector[clockHandle - 1] = pageName; // Fix the evicted Index becaus ei think doesnt exist now
    } else {
        clockVector[numPages] = pageName;
    }

    numPages++;
    return std::make_tuple(0, evictedBuffer);
}

// Returns the page buffer of the evicted page
uint64_t *BufferPool::evict()
{
	//assert bufferpool is full??
	std::string currPageName = clockVector[clockHandle];
    std::optional<HashMap::Node *> searchResult;
    HashMap::Node *currNode;
    uint64_t *evictedBuffer;

    //assert(searchResult.has_value());
    bool notFound = true;
    while (notFound){
        searchResult = hashMap->search(currPageName);
        if (searchResult.has_value()) {
            currNode = searchResult.value();
        }
        if (currNode->accessBit){
            currNode->accessBit = false;
        } else {
            notFound = false;
            evictedBuffer = currNode->page;
            delete currNode;
        }
        clockHandle = (clockHandle + 1) % numPages;
    }

    return evictedBuffer;
}

/**
 * @brief Constructor for the HashMap class
 */

HashMap::HashMap(size_t initial, size_t maxDir)
    : bucketOverflowThreshold(BUCKET_OVERFLOW_THRESHOLD),
      numBitsUsed(0),
      maxDirSize(maxDir)
{
    numBitsUsed = ceil(log2(initial));

    size_t dirSize = 1ULL << numBitsUsed; // 2^numBitsUsed

    for (size_t i = 0; i < dirSize; i++)
    {
        DirEntry *entry = new DirEntry();
        entry->numHashedDigits = numBitsUsed;
        entry->hashedIndex = i;
        directory.push_back(entry);
    }
}

/**
 * @brief Destructor for the HashMap class
 */

HashMap::~HashMap()
{
    // directory may contain duplicated DirEntry* entries from extendible hashing
    // delete each unique DirEntry once, and delete all Nodes in its chain.
    std::unordered_set<DirEntry *> seen;

    for (DirEntry *entry : directory)
    {
        if (!entry)
        {
            continue;
        }

        if (seen.insert(entry).second)
        { // only is true for the first time we encounter this pointer, so we only run the if-body once per unique pointer
            Node *curr = entry->first;
            while (curr)
            {
                delete curr;
                curr = curr->next;
            }
            delete entry;
        }
    }
    directory.clear();
}

uint64_t HashMap::hashFunction(std::string key)
{
    // CITE THE GITHUB LINK: https://github.com/stbrumme/xxhash/blob/master/xxhash64.h
    uint64_t h = XXHash64::hash(key.data(), key.size(), 0); // 0 is the seed
    return h;
}

// Maybe change return value
void HashMap::insertNodeToBucket(Node *node, DirEntry *dirEntry)
{
    if (!dirEntry->first)
    {
        dirEntry->first = node;
        dirEntry->tail = node;
    }

    else
    {
        dirEntry->tail->next = node;
        dirEntry->tail = node;
    }
    dirEntry->chainSize++;
}

// Maybe change return value
void HashMap::rehashBucket(DirEntry *dirEntry)
{
    Node *chainCurrent = dirEntry->first;
    DirEntry *newEntry = new DirEntry();

    // Adds 1 immediately left to old hashedIndex of dirEntry
    // Ex. dirEntry->hashedIndex = 4 (0100), newEntry->hashedIndex = 12 (1100)
    newEntry->hashedIndex = dirEntry->hashedIndex | (1ULL << dirEntry->numHashedDigits);

    // Counts num of bits in current hashed index
    // int nBits = static_cast<int>(std::floor(std::log2(numBitsUsed))) + 1;

    // Creates all indices that are point to bucket being rehashed
    // Creates prefixes that will be 'OR'ed to current hashedIndex to generate each index

    for (size_t prefix = 0; prefix < (1ULL << (numBitsUsed - dirEntry->numHashedDigits)); ++prefix)
    {
        PRINT("hmm");
        size_t combined = (prefix << dirEntry->numHashedDigits) | dirEntry->hashedIndex;
        // Assigns all indices that start with 0 to old dirEntry
        if (((combined >> (numBitsUsed - 1)) & 1) == 0)
        {
            directory[combined] = dirEntry;
        }
        else
        { // Assigns all indices that start with 1 to new dirEntry
            directory[combined] = newEntry;
        }
    }

    dirEntry->numHashedDigits++;
    newEntry->numHashedDigits = dirEntry->numHashedDigits;

    dirEntry->first = nullptr;
    dirEntry->tail = nullptr;

    while (chainCurrent)
    {
        PRINT("In chain current while loop - Anbu");
        uint64_t hashedPageName = hashFunction(chainCurrent->pageName);
        uint64_t mask = (1ULL << numBitsUsed) - 1;

        uint64_t maskedHashPage = hashedPageName & mask;

        DirEntry *newEntry = directory[maskedHashPage];

        insertNodeToBucket(chainCurrent, newEntry);
        chainCurrent = chainCurrent->next;
    }
}

int HashMap::extendDir()
{
    size_t currDirSize = directory.size();
    size_t newDirSize = 2 * currDirSize;
    if (newDirSize > maxDirSize)
    {
        return 1; // since we can't go past the maximum allowed directory size
    }
    directory.resize(newDirSize);
    for (size_t i = 0; i < currDirSize; i++)
    {
        directory[i + currDirSize] = directory[i];
    }
    numBitsUsed++;
    return 0;
}

int HashMap::insert(std::string pageName, uint64_t *page, size_t pageSize)
{
    uint64_t hashedPageName = hashFunction(pageName);
    uint64_t mask = (1ULL << numBitsUsed) - 1;

    uint64_t maskedHashPage = hashedPageName & mask;

    DirEntry *dirEntry = directory[maskedHashPage];
    Node *insertNode = new Node(pageName, page, pageSize);

    if (dirEntry->chainSize >= size_t(bucketOverflowThreshold))
    {
        if (dirEntry->numHashedDigits < numBitsUsed)
        {
            // rehash buckets
            rehashBucket(dirEntry);
        }
        else
        {
            // extend directory + rehash buckets :)
            // extend directory ->
            int extendDirResult = extendDir();
            if (extendDirResult == 1)
            {
                delete insertNode;
                return 1; // i.e. we have exceeded the directory size
            }
            rehashBucket(dirEntry);
        }
        // we need to recompute the target bucket after any rehash/extending that happens
        hashedPageName = hashFunction(pageName);
        mask = (numBitsUsed == 0) ? 0 : ((1ULL << numBitsUsed) - 1);
        maskedHashPage = hashedPageName & mask;
        dirEntry = directory[maskedHashPage];
    }
    insertNodeToBucket(insertNode, dirEntry);
    return 0; // insert success
}

std::optional<HashMap::Node *> HashMap::search(std::string pageName)
{

    uint64_t hashedPageName = hashFunction(pageName);
    uint64_t mask = (1ULL << numBitsUsed) - 1;
    uint64_t maskedHashPage = hashedPageName & mask;
    DirEntry *dirEntry = directory[maskedHashPage];

    if (!dirEntry->first)
    {
        return std::nullopt; // this means the directory entry itself is empty
    }

    else
    {
        Node *curr = dirEntry->first;

        while (curr)
        {
            if (curr->pageName == pageName)
            {
                return curr;
            }
            curr = curr->next;
        }
        return std::nullopt; // if we reach here, that means there is no node that has a matching page name
    }
}

std::optional<HashMap::Node *> HashMap::remove(std::string pageName)
{
    uint64_t hashedPageName = hashFunction(pageName);
    uint64_t mask = (1ULL << numBitsUsed) - 1;
    uint64_t maskedHashPage = hashedPageName & mask;
    DirEntry *dirEntry = directory[maskedHashPage];

    if (!dirEntry->first)
    {
        return std::nullopt; // this means the directory entry itself is empty
    }

    else
    {
        Node *curr = dirEntry->first;
        Node *prev = nullptr;
        Node *removeNode = nullptr;
        while (curr)
        {
            if (curr->pageName == pageName)
            {
                removeNode = curr;
                if (prev == nullptr)
                { // i.e. we are at first
                    dirEntry->first = curr->next;
                    return removeNode;
                }

                else if (curr == dirEntry->tail)
                {
                    prev->next = curr->next;
                    dirEntry->tail = prev;
                    return removeNode;
                }

                else
                {
                    prev->next = curr->next;
                    return removeNode;
                }
            }
            prev = curr;
            curr = curr->next;
        }
        return std::nullopt; // if we reach here, that means there is no node that has a matching page name
    }
}
