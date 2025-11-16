#include "bufferpool.hpp";
#include "xxhash64.h";
#include <unordered_set>;
#include <cmath>;

/**
 * @brief Constructor for the BufferPool class
 */

BufferPool::BufferPool(size_t initialAmount, size_t maximalAmount, size_t maxPagesAmount)
    : hashMap(initialAmount, maximalAmount),
      initialDirSize(initialAmount),
      maxDirSize(maximalAmount),
      maxPages(maxPagesAmount) {};

/**
 * @brief Destructor for the BufferPool class
 */
BufferPool::~BufferPool()
{
}

std::optional<std::tuple<int, uint64_t *>> BufferPool::searchPage(int sstNum, int pageOffset)
{
    std::string key = std::to_string(sstNum) + "_" + std::to_string(pageOffset);
    std::optional<HashMap::Node *> searchResult = hashMap.search(key);

    if (searchResult.has_value())
    {
        HashMap::Node *node = searchResult.value();
        std::tuple<int, uint64_t *> returnValue = std::make_tuple(node->pageSize, node->page);
        return returnValue;
    }
    else {
        return std::nullopt;
    }
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
    int currDirSize = directory.size();
    int newDirSize = 2 * currDirSize;
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

    if (dirEntry->chainSize >= bucketOverflowThreshold)
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
                return 1; // i.e. we have exceeded the directory size
            }
            rehashBucket(dirEntry);
        }
        insertNodeToBucket(insertNode, dirEntry);
    }
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
        if (!removeNode)
        {
            return std::nullopt; // if we reach here, that means there is no node that has a matching page name
        }
    }
}