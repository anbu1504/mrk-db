#include "bufferpool.hpp";
#include "xxhash64.h";
#include <cmath>;

/**
 * @brief Constructor for the BufferPool class
 */

BufferPool::BufferPool(size_t initialAmount, size_t maximalAmount, size_t maxPagesAmount)
    : hashMap(initialAmount),
    initialDirSize(initialAmount),
    maxDirSize(maximalAmount),
    maxPages(maxPagesAmount) {};

/**
 * @brief Destructor for the BufferPool class
 */
BufferPool::~BufferPool()
{
}

std::optional<std::tuple<int, uint64_t *>> BufferPool::searchPage(int sstNum, int pageOffset){
    std::string key = std::to_string(sstNum) + "_" + std::to_string(pageOffset);
    std::optional<std::tuple<int, uint64_t *>> searchResult = hashMap.search(key);

    if (searchResult.has_value()){
        return searchResult.value();
    } else {
        return std::nullopt;
    }
}

int addPage(uint64_t *buffer){
    
}







/**
 * @brief Constructor for the HashMap class
 */

HashMap::HashMap(size_t initial)
{
    numBitsUsed = std::ceil(std::log2(initial)); // for global depth of directory
    bufferOverflowThreshold = 3;

    // Initializing buckets

    // FIXME: Update buckets to new system

    // for (size_t i = 0; i < initial; i++)
    // {
    //     directory[i] = std::make_tuple(
    //         i,
    //         nullptr,
    //         0);
    // };
}

/**
 * @brief Destructor for the HashMap class
 */

HashMap::~HashMap()
{
    // for (auto &[depth, head, count] : directory)
    // {
    //     Node *curr = head;
    //     while (curr)
    //     {
    //         Node *tempNext = curr->next;
    //         delete curr;
    //         curr = tempNext;
    //     }
    // }
}

uint64_t HashMap::hashFunction(std::string key)
{
    // CITE THE GITHUB LINK: https://github.com/stbrumme/xxhash/blob/master/xxhash64.h
    uint64_t h = XXHash64::hash(key.data(), key.size(), 0); // 0 is the seed
    return h;
}

//Maybe change return value
void HashMap::insertNodeToBucket(Node *node, DirEntry *dirEntry){
    if (!dirEntry->first)
    {
        dirEntry->first = node;
        dirEntry->tail = node;
    }

    else {
        dirEntry->tail->next = node;
        dirEntry->tail = node;
    }
    dirEntry->chainSize++;
}

// Maybe change return value
void HashMap::rehashBucket(DirEntry *dirEntry){
    Node *chainCurrent = dirEntry->first;
    DirEntry* newEntry = new DirEntry();

    // Adds 1 immediately left to old hashedIndex of dirEntry
    // Ex. dirEntry->hashedIndex = 4 (0100), newEntry->hashedIndex = 12 (1100)
    newEntry->hashedIndex =  dirEntry->hashedIndex | (1ULL << dirEntry->numHashedDigits);

    // Counts num of bits in current hashed index
    // int nBits = static_cast<int>(std::floor(std::log2(numBitsUsed))) + 1;

    // Creates all indices that are point to bucket being rehashed
    // Creates prefixes that will be 'OR'ed to current hashedIndex to generate each index
    for (size_t prefix = 0; prefix < (1ULL << (numBitsUsed - dirEntry->numHashedDigits)); ++prefix) {
        size_t combined = (prefix << dirEntry->numHashedDigits) | dirEntry->hashedIndex;
        // Assigns all indices that start with 0 to old dirEntry
        if (((combined >> (numBitsUsed - 1)) & 1) == 0){
            directory[combined] = dirEntry;
        } else{  // Assigns all indices that start with 1 to new dirEntry
            directory[combined] = newEntry;
        }
    }

    dirEntry->numHashedDigits++;
    newEntry->numHashedDigits = dirEntry->numHashedDigits;

    dirEntry->first = nullptr;
    dirEntry->tail = nullptr;

    while (!chainCurrent){
        uint64_t hashedPageName = hashFunction(chainCurrent->pageName);
        uint64_t mask = (1ULL << numBitsUsed) - 1;

        uint64_t maskedHashPage = hashedPageName & mask;

        DirEntry *newEntry = directory[maskedHashPage];

        insertNodeToBucket(chainCurrent, newEntry);
        chainCurrent = chainCurrent->next;

    }

}

int HashMap::extendDir() {
    int currDirSize = 1 << numBitsUsed; // 2^numBitsUsed
    int newDirSize = 2 * currDirSize;

    
}


int HashMap::insert(std::string pageName, uint64_t *page)
{
    uint64_t hashedPageName = hashFunction(pageName);
    uint64_t mask = (1ULL << numBitsUsed) - 1;

    uint64_t maskedHashPage = hashedPageName & mask;

    DirEntry *dirEntry = directory[maskedHashPage];
    Node *insertNode = new Node(pageName, page);

    insertNodeToBucket(insertNode, dirEntry);


    if (dirEntry->chainSize >= bufferOverflowThreshold)
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
            extendDir();
            rehashBucket(dirEntry);
        }
    }
}
