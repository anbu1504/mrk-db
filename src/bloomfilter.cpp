#include "../include/bloomfilter.hpp"
#include "../external/xxhash64.h"

/**
 * @brief Constructor for the BloomFilter class
 */

BloomFilter::BloomFilter(BufferPool* bufPool, uint64_t sstNum, uint64_t numKeys, uint64_t pageOffset)
    : bufPool(bufPool),
      sstNum(sstNum),
      numKeys(numKeys),
      pageOffset(pageOffset),
      bitsPerEntry(bitsPerEntry) 
{
    numHashFunctions = static_cast<uint64_t>(bitsPerEntry * LN_2); // number of hash functions calculation from slides (M * ln(2))
    totalBits = numKeys * bitsPerEntry;
    numItems = numKeys * 2;
}


/**
 * @brief Destructor for the BloomFilter class
 */
BloomFilter::~BloomFilter() {}


void BloomFilter::addKey(uint64_t key) {
    PageBuffer pageBuf;

    for (uint64_t hashSeed = 0; hashSeed < numHashFunctions; hashSeed++) {
        uint64_t hashValue = XXHash64::hash(&key, sizeof(uint64_t), hashSeed) % totalBits;

        uint64_t pageNum = hashValue / (UINT64S_PER_PAGE * UINT64_SIZE_BITS);
        uint64_t wordIndex = (hashValue / UINT64_SIZE_BITS) % UINT64S_PER_PAGE;
        uint64_t bitNumber = hashValue % UINT64_SIZE_BITS;

        // read the page first
        bufPool->bread(sstNum, pageOffset + pageNum, pageBuf);

        // set the bit
        pageBuf[wordIndex] |= (1ULL << bitNumber);

        // write updated page
        bufPool->bwrite(sstNum, pageOffset + pageNum, pageBuf);
    }
}


void BloomFilter::addMultiKeys(uint64_t* memtableData) {
    // since memtableData is in the form <key, value, key, value>
    // every even index will be a key
    
    for (uint64_t i = 0; i < numItems; i++) {
        if (i % 2 == 0) { // even index
            addKey(memtableData[i]);
        }
    }
}

bool BloomFilter::checkKey(uint64_t key) {
    PageBuffer pageBuf;

    for (uint64_t hashSeed = 0; hashSeed < numHashFunctions; hashSeed++) {
        uint64_t hashValue = XXHash64::hash(&key, sizeof(uint64_t), hashSeed) % totalBits;
        
        uint64_t absoluteBitNum = hashValue / UINT64_SIZE_BITS;
        uint64_t pageNum = hashValue / UINT64S_PER_PAGE; // which page I should go to
        uint64_t intArrive = pageNum % UINT64S_PER_PAGE; // which uint64_t should I go to
        
        uint64_t bitNumber = hashValue % UINT64_SIZE_BITS; // the actual bit to arrive at

        bufPool->bread(sstNum, pageOffset + pageNum, pageBuf);
        if (!((pageBuf[intArrive] >> bitNumber) & (1ULL))) {
            return false;
        }
    }
    return true;
}

void BloomFilter::wipePages() {
    uint64_t numPages = CEIL_DIV(totalBits, PAGE_SIZE * 8);
    PageBuffer pageBuf; 
    
    memset(pageBuf, 0, PAGE_SIZE);
    
    for (uint64_t page = 0; page < numPages; page++) {
        bufPool->bwrite(sstNum, pageOffset + page, pageBuf);
    }
}