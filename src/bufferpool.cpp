#include "../include/bufferpool.hpp"
#include "../external/xxhash64.h"

#include <cmath>
#include <unordered_set>
#include <unistd.h>
#include <fcntl.h>
#include <fstream>
#include <cstring>
#include <iostream>
#include <sstream>
#include <cstdio>

#define BUCKET_OVERFLOW_THRESHOLD 4

/**
 * @brief Constructor for the HashMap class
 */

HashMap::HashMap(size_t initial, size_t maxDir)
    : bucketOverflowThreshold(BUCKET_OVERFLOW_THRESHOLD), numBitsUsed(0), maxDirSize(maxDir) {
    numBitsUsed = ceil(log2(initial));

    size_t dirSize = 1ULL << numBitsUsed;  // 2^numBitsUsed

    for (size_t i = 0; i < dirSize; i++) {
        DirEntry* entry = new DirEntry();
        entry->numHashedDigits = numBitsUsed;
        entry->hashedIndex = i;
        directory.push_back(entry);
    }
}

/**
 * @brief Destructor for the HashMap class
 */

HashMap::~HashMap() {
    // directory may contain duplicated DirEntry* entries from extendible hashing
    // delete each unique DirEntry once, and delete all Nodes in its chain.
    std::unordered_set<DirEntry*> seen;

    for (DirEntry* entry : directory) {
        if (!entry) {
            continue;
        }

        if (seen.insert(entry).second) {  // only is true for the first time we encounter this pointer, so we only run
                                          // the if-body once per unique pointer
            Node* curr = entry->first;
            while (curr) {
                delete curr;
                curr = curr->next;
            }
            delete entry;
        }
    }
    directory.clear();
}

uint64_t HashMap::hashFunction(std::string key) {
    // CITE THE GITHUB LINK: https://github.com/stbrumme/xxhash/blob/master/xxhash64.h
    uint64_t h = XXHash64::hash(key.data(), key.size(), 0);  // 0 is the seed
    return h;
}

// Maybe change return value
void HashMap::insertNodeToBucket(Node* node, DirEntry* dirEntry) {
    if (!dirEntry->first) {
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
void HashMap::rehashBucket(DirEntry* dirEntry) {
    Node* chainCurrent = dirEntry->first;
    DirEntry* newEntry = new DirEntry();

    // Adds 1 immediately left to old hashedIndex of dirEntry
    // Ex. dirEntry->hashedIndex = 4 (0100), newEntry->hashedIndex = 12 (1100)
    newEntry->hashedIndex = dirEntry->hashedIndex | (1ULL << dirEntry->numHashedDigits);

    // Creates all indices that are point to bucket being rehashed
    // Creates prefixes that will be 'OR'ed to current hashedIndex to generate each index

    for (size_t prefix = 0; prefix < (1ULL << (numBitsUsed - dirEntry->numHashedDigits)); ++prefix) {
        size_t combined = (prefix << dirEntry->numHashedDigits) | dirEntry->hashedIndex;
        // Assigns all indices that start with 0 to old dirEntry
        if (((combined >> (numBitsUsed - 1)) & 1) == 0) {
            directory[combined] = dirEntry;
        } else {  // Assigns all indices that start with 1 to new dirEntry
            directory[combined] = newEntry;
        }
    }

    dirEntry->numHashedDigits++;
    newEntry->numHashedDigits = dirEntry->numHashedDigits;

    dirEntry->first = nullptr;
    dirEntry->tail = nullptr;

    while (chainCurrent) {
        uint64_t hashedPageName = hashFunction(chainCurrent->pageName);
        uint64_t mask = (1ULL << numBitsUsed) - 1;

        uint64_t maskedHashPage = hashedPageName & mask;

        DirEntry* newEntry = directory[maskedHashPage];

        Node* tempNext = chainCurrent->next;

        chainCurrent->next = nullptr;
        insertNodeToBucket(chainCurrent, newEntry);
        chainCurrent = tempNext;
    }
}

int HashMap::extendDir() {
    size_t currDirSize = directory.size();
    size_t newDirSize = 2 * currDirSize;
    if (newDirSize > maxDirSize) {
        return 1;  // since we can't go past the maximum allowed directory size
    }
    directory.resize(newDirSize);
    for (size_t i = 0; i < currDirSize; i++) {
        directory[i + currDirSize] = directory[i];
    }
    numBitsUsed++;
    return 0;
}

int HashMap::insert(std::string pageName, uint64_t* page, size_t pageSize) {
    uint64_t hashedPageName = hashFunction(pageName);
    uint64_t mask = (1ULL << numBitsUsed) - 1;

    uint64_t maskedHashPage = hashedPageName & mask;

    DirEntry* dirEntry = directory[maskedHashPage];
    Node* insertNode = new Node(pageName, page, pageSize);

    if (dirEntry->chainSize >= size_t(bucketOverflowThreshold)) {
        if (dirEntry->numHashedDigits < numBitsUsed) {
            // rehash buckets
            rehashBucket(dirEntry);
        } else {
            // extend directory + rehash buckets
            int extendDirResult = extendDir();
            if (extendDirResult == 0) {
                rehashBucket(dirEntry);
            }
            // If directory can't be extended
            // Ignore bucket threshold and add to chain of hashed bucket
            
        }
        // we need to recompute the target bucket after any rehash/extending that happens
        hashedPageName = hashFunction(pageName);
        mask = (1ULL << numBitsUsed) - 1;
        maskedHashPage = hashedPageName & mask;
        dirEntry = directory[maskedHashPage];
    }
    insertNodeToBucket(insertNode, dirEntry);
    return 0;  // insert success
}

std::optional<HashMap::Node*> HashMap::search(std::string pageName) {
    uint64_t hashedPageName = hashFunction(pageName);
    uint64_t mask = (1ULL << numBitsUsed) - 1;
    uint64_t maskedHashPage = hashedPageName & mask;
    DirEntry* dirEntry = directory[maskedHashPage];

    if (!dirEntry->first) {
        return std::nullopt;  // this means the directory entry itself is empty
    }

    else {
        Node* curr = dirEntry->first;

        while (curr) {
            if (curr->pageName == pageName) {
                return curr;
            }
            curr = curr->next;
        }
        return std::nullopt;  // if we reach here, that means there is no node that has a matching page name
    }
}

std::optional<HashMap::Node*> HashMap::remove(std::string pageName) {
    uint64_t hashedPageName = hashFunction(pageName);
    uint64_t mask = (1ULL << numBitsUsed) - 1;
    uint64_t maskedHashPage = hashedPageName & mask;
    DirEntry* dirEntry = directory[maskedHashPage];

    if (!dirEntry->first) {
        return std::nullopt;  // this means the directory entry itself is empty
    }

    else {
        Node* curr = dirEntry->first;
        Node* prev = nullptr;
        Node* removeNode = nullptr;
        while (curr) {
            if (curr->pageName == pageName) {
                removeNode = curr;
                if (prev == nullptr) {  // i.e. we are at first
                    dirEntry->first = curr->next;
                    return removeNode;
                }

                else if (curr == dirEntry->tail) {
                    prev->next = curr->next;
                    dirEntry->tail = prev;
                    return removeNode;
                }

                else {
                    prev->next = curr->next;
                    return removeNode;
                }
            }
            prev = curr;
            curr = curr->next;
        }
        return std::nullopt;  // if we reach here, that means there is no node that has a matching page name
    }
}


/**
 * @brief Constructor for the BufferPool class
 */

BufferPool::BufferPool(size_t initialDirSizeVal, size_t maxDirSizeVal, size_t maxPagesVal, std::string dbNameVal)
    : initialDirSize(initialDirSizeVal),
      maxDirSize(maxDirSizeVal),
      maxPages(maxPagesVal),
      numPages(0),
      hashMap(new HashMap(initialDirSizeVal, maxDirSizeVal)),
      clockHandle(0) {
    clockVector.resize(maxPagesVal);
};

/**
 * @brief Destructor for the BufferPool class
 */
BufferPool::~BufferPool() {
    delete hashMap;
}

HashMap::Node* BufferPool::searchPage(std::string pageName) {
    std::optional<HashMap::Node*> searchResult = hashMap->search(pageName);

    if (searchResult.has_value()) {
        HashMap::Node* node = searchResult.value();
        node->accessBit = true;
        return node;
    } else {
        return nullptr;
    }
}

void BufferPool::compactClockVector(){
    size_t writeIndex = 0;
    for (size_t readIndex = 0; readIndex < clockVector.size(); readIndex++) {
        if (clockVector[readIndex] != "") {
            clockVector[writeIndex++] = clockVector[readIndex];
        }
    }
    numPages = writeIndex;
    if (clockHandle >= numPages) {
        clockHandle = 0;
    }
}

// Return Value: Buffer of evicted page (or nullptr)
HashMap::Node* BufferPool::addPage(std::string pageName, PageBuffer buffer) {
    // Assert that this page is not in bufferpool already?
    HashMap::Node* evictedNode = nullptr;

    if (numPages == maxPages) {
        evictedNode = evictFromBpool();
        numPages = numPages - 1;
    }

    hashMap->insert(pageName, buffer, PAGE_SIZE);

    if (evictedNode) {
        clockVector[clockHandle - 1] = pageName;
    } else {
        clockVector[numPages] = pageName;
    }

    numPages++;
    return evictedNode;
}

// Returns the node of the evicted page
HashMap::Node* BufferPool::evictFromBpool() {
    std::string currPageName = clockVector[clockHandle];
    std::optional<HashMap::Node*> searchResult;
    HashMap::Node* currNode = nullptr;

    bool notFound = true;
    while (notFound) {
        searchResult = hashMap->search(currPageName);
        if (searchResult.has_value()) {
            currNode = searchResult.value();
        } // this if statement should always be true

        if (currNode->accessBit) {
            currNode->accessBit = false;
        } else {
            notFound = false;
            hashMap->remove(currPageName);
            clockVector[clockHandle] = "";
        }
        clockHandle = (clockHandle + 1) % numPages;
    }

    return currNode;
}

std::string BufferPool::makeName(std::string filename, uint64_t pageNum) {
    return filename + "_" + std::to_string(pageNum);
}

void BufferPool::evictNode(HashMap::Node* node){
    if (node->dirtyBit){
        std::string pageName = node->pageName;
        size_t underScorePos = pageName.find('_');
        std::string filename = pageName.substr(0, underScorePos);
        std::string pageNumStr = pageName.substr(underScorePos + 1);
        int pageNum = std::stoi(pageNumStr);

        int fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_DIRECT, 0644);
        if (fd == -1) {
            close(fd);
            throw std::runtime_error(std::string("open failed: ") + std::strerror(errno));
        }

        ssize_t written = pwrite(fd, node->page, PAGE_SIZE, pageNum * PAGE_SIZE);
        if (written != PAGE_SIZE) {
            close(fd);
            throw std::runtime_error(std::string("write did not write a page: ") + std::strerror(errno));
        }

        close(fd);
    }

    std::free(node->page);
    delete node;
}

ssize_t BufferPool::bread(uint64_t sstNum, uint64_t pageNum, PageBuffer buffer, bool bypassCache = false){
    return bread(std::to_string(sstNum), pageNum, buffer, bypassCache);
}

ssize_t BufferPool::bread(std::string filename, uint64_t pageNum, PageBuffer buffer, bool bypassCache = false) {
    std::string pageName = makeName(filename, pageNum);
    HashMap::Node* readNode = searchPage(pageName);

    ssize_t bytesRead;

    if (!readNode) {
        int fd = open(SST_PATH(filename).c_str(), O_RDONLY | O_DIRECT);
        if (fd == -1) {
            close(fd);
            throw std::runtime_error(std::string("open failed: ") + std::strerror(errno));
        }
        bytesRead = pread(fd, buffer, PAGE_SIZE, pageNum * PAGE_SIZE);

        if (!bypassCache){
            uint64_t* bufferHeap = static_cast<uint64_t*>(std::malloc(PAGE_SIZE));
            memcpy(bufferHeap, buffer, PAGE_SIZE);

            if (bytesRead != 0){
                PRINT("Reading something not page aligned for some reason"); // debug statement
                memset(bufferHeap + bytesRead, 0, PAGE_SIZE - bytesRead);
            }

            HashMap::Node* evictedNode = addPage(pageName, bufferHeap);

            if (evictedNode) {
                evictNode(evictedNode);
            }
        }
        close(fd);
        return PAGE_SIZE;
    }

    else {
        uint64_t* bufferFromBP = readNode->page;
        memcpy(buffer, bufferFromBP, PAGE_SIZE);
        return PAGE_SIZE;
    }
}

ssize_t BufferPool::bwrite(uint64_t sstNum, uint64_t pageNum, PageBuffer buffer) {
    return bwrite(std::to_string(sstNum), pageNum, buffer);
}

ssize_t BufferPool::bwrite(std::string filename, uint64_t pageNum, PageBuffer buffer) {
    std::string pageName = makeName(filename, pageNum);
    HashMap::Node* writeNode = searchPage(pageName);
    
    if (!writeNode){
        uint64_t* bufferHeap = static_cast<uint64_t*>(std::malloc(PAGE_SIZE));
        memcpy(bufferHeap, buffer, PAGE_SIZE);

        HashMap::Node* evictedNode = addPage(pageName, bufferHeap);

        if (evictedNode) {
            evictNode(evictedNode);
        }

        return PAGE_SIZE;
    }
    else {
        memcpy(buffer, writeNode->page, PAGE_SIZE);
        writeNode->dirtyBit = true;
        return PAGE_SIZE;
    }
}

void BufferPool::evictAllPages() {
    HashMap::Node *currNode;
    while (numPages > 0) {
        currNode = evictFromBpool();
        evictNode(currNode);
        numPages = numPages - 1;
    }
}

void BufferPool::bdelete(std::string filename){
    std::vector<std::string> deletePageNames;
    for (std::string& pageName: clockVector){
        if (pageName.rfind(filename + "_", 0) == 0){
            deletePageNames.push_back(pageName);
            pageName = "";
        }
    }

    HashMap::Node* deleteNode;
    for (std::string deletingPageName: deletePageNames){
        std::optional<HashMap::Node*> deleteResult = hashMap->remove(deletingPageName);
        if (deleteResult.has_value()){
            deleteNode = deleteResult.value();
        } // This if statement should always be true
        
        evictNode(deleteNode);
    }

    compactClockVector();
    std::string filepath = SST_PATH(filename);
    remove(filepath.c_str());


}
