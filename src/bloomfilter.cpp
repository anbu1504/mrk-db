#include "../include/bloomfilter.hpp"
#include "../external/xxhash64.h"

/**
 * @brief Constructor for the BloomFilter class
 */

BloomFilter::BloomFilter(BufferPool* bufPool, uint64_t sstNum, uint64_t numKeys, uint64_t pageOffset, uint64_t bitsPerEntry)
    : bufPool(bufPool),
      sstNum(sstNum),
      numKeys(numKeys),
      pageOffset(pageOffset),
      bitsPerEntry(bitsPerEntry) 
{
    numHashFunctions = static_cast<uint64_t>(bitsPerEntry * LN_2); // number of hash functions calculation from slides (M * ln(2))
    totalBits = numKeys * bitsPerEntry;
}


/**
 * @brief Destructor for the BloomFilter class
 */
BloomFilter::~BloomFilter() {}


void BloomFilter::addKey(uint64_t key) {
    // use knowledge that a file of uint64s is the same as saying a file of bits
    // each uint64 is 64 bits, a file of those is a whole lot of 1's and 0's
    // so if i have 5 uint64s in a page 
}

void BloomFilter::addMultiKeys(uint64_t* memtableData) {

}

bool BloomFilter::checkKey(uint64_t key) {
    PageBuffer pageBuf;

    for (uint64_t hashSeed = 0; hashSeed < numHashFunctions; hashSeed++) {
        uint64_t hashValue = XXHash64::hash(&key, sizeof(uint64_t), hashSeed) % totalBits;
        
        uint64_t pageNum = hashValue / UINT64S_PER_PAGE; // which page I should go to
        uint64_t intArrive = pageNum % UINT64S_PER_PAGE; // which uint64_t should I go to
        uint64_t bitNumber = hashValue % UINT64_SIZE_BITS; // 

        // bufPool->bread(sstNum, )
    }
}