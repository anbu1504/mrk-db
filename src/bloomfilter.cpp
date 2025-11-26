#include "../include/bloomfilter.hpp"

#include <iostream>

#include "../external/xxhash64.h"

/**
 * @brief Constructor #1 for the BloomFilter class
 */
BloomFilter::BloomFilter(size_t num_keys)
    : total_bits(num_keys * BITS_PER_ENTRY),
      filter(CEIL_DIV(num_keys * BITS_PER_ENTRY, PAGE_SIZE * 8) * UINT64S_PER_PAGE),
      num_bitsets_initialized(0) {}

/**
 * @brief Destructor for the BloomFilter class
 */
BloomFilter::~BloomFilter() {}

void BloomFilter::addKey(uint64_t key) {
    for (int hash_seed = 0; hash_seed < NUM_HASH_FUNCS; hash_seed++) {
        uint64_t hash_val = XXHash64::hash(&key, sizeof(uint64_t), hash_seed) % total_bits;

        uint64_t bitset_num = hash_val / BITSET_SIZE;
        uint64_t bit_num = hash_val % BITSET_SIZE;

        filter[bitset_num] |= (1ULL) << bit_num;
    }
}

void BloomFilter::initFromBuf(uint64_t* pageBuf) {
    size_t numRead = 0;
    // stop when either num_bitsets_initialized >= filter.size() or we've read >= ULLONGS_PER_PAGE
    while (num_bitsets_initialized < filter.size() && numRead < ULLONGS_PER_PAGE) {
        // std::bitset<BITSET_SIZE> temp_bitset(pageBuf[numRead]);
        filter[num_bitsets_initialized] = pageBuf[numRead];

        num_bitsets_initialized++;
        numRead++;
    }
}

void BloomFilter::initFromSST(BufferPool* bufPool, int filterStart, std::string filename, int sstNum) {
    uint64_t pageBuf[UINT64S_PER_PAGE];

    for (uint64_t pageNum = filterStart; pageNum < filterStart + getNumPages(); pageNum++) {
        bufPool->comboRead(filename, sstNum, PAGE_SIZE * pageNum, pageBuf, PAGE_SIZE);

        initFromBuf(pageBuf);
    }
}

bool BloomFilter::checkKey(uint64_t key) {
    for (int hash_seed = 0; hash_seed < NUM_HASH_FUNCS; hash_seed++) {
        uint64_t hash_val = XXHash64::hash(&key, sizeof(uint64_t), hash_seed) % total_bits;

        uint64_t bitset_num = hash_val / BITSET_SIZE;
        uint64_t bit_num = hash_val % BITSET_SIZE;

        if (!((filter[bitset_num] >> bit_num) & (1ULL))) {
            return false;
        }
    }
    return true;
}

bool BloomFilter::checkKey2(uint64_t key, BufferPool* bufPool, int filterStart, std::string filename, int sstNum) {
    uint64_t pageBuf[UINT64S_PER_PAGE];

    for (int hash_seed = 0; hash_seed < NUM_HASH_FUNCS; hash_seed++) {
        uint64_t hash_val = XXHash64::hash(&key, sizeof(uint64_t), hash_seed) % total_bits;

        uint64_t absolute_bitset_num = hash_val / BITSET_SIZE;
        uint64_t page_num = absolute_bitset_num / UINT64S_PER_PAGE;
        uint64_t bitset_num = absolute_bitset_num % UINT64S_PER_PAGE;

        uint64_t bit_num = hash_val % BITSET_SIZE;

        bufPool->comboRead(filename, sstNum, PAGE_SIZE * (filterStart + page_num), pageBuf, PAGE_SIZE);

        if (!((pageBuf[bitset_num] >> bit_num) & (1ULL))) {
            return false;
        }
    }
    return true;
}

std::vector<uint64_t> BloomFilter::flattenBloomFilter() {
    std::vector<uint64_t> output;

    for (size_t bitset_num = 0; bitset_num < filter.size(); bitset_num++) {
        output.push_back(filter[bitset_num]);
    }

    return output;
}

uint64_t BloomFilter::getTotalBits() { return total_bits; }

uint64_t BloomFilter::getNumPages() { return CEIL_DIV(total_bits, PAGE_SIZE * 8); }
