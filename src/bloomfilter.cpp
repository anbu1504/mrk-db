#include "../include/bloomfilter.hpp"

#include <iostream>

#include "../external/xxhash64.h"

/**
 * @brief Constructor #1 for the BloomFilter class
 */
BloomFilter::BloomFilter(size_t num_keys)
    : total_bits(num_keys * BITS_PER_ENTRY),
      filter(CEIL_DIV(num_keys * BITS_PER_ENTRY, BITSET_SIZE)),
      num_bitsets_initialized(CEIL_DIV(num_keys * BITS_PER_ENTRY, BITSET_SIZE)) {}

/**
 * @brief Constructor #2 for the BloomFilter class
 */
BloomFilter::BloomFilter(uint64_t total_bits)
    : total_bits(total_bits), filter(CEIL_DIV(total_bits, BITSET_SIZE)), num_bitsets_initialized(0) {}

/**
 * @brief Destructor for the BloomFilter class
 */
BloomFilter::~BloomFilter() {}

void BloomFilter::addKey(uint64_t key) {
    for (int hash_seed = 0; hash_seed < NUM_HASH_FUNCS; hash_seed++) {
        uint64_t hash_val = XXHash64::hash(&key, sizeof(uint64_t), hash_seed) % total_bits;

        uint64_t bitset_num = hash_val / BITSET_SIZE;
        uint64_t bit_num = hash_val % BITSET_SIZE;

        filter[bitset_num].set(bit_num);
    }
}

void BloomFilter::initFromBuf(unsigned long long* pageBuf) {
    size_t numRead = 0;
    // stop when either num_bitsets_initialized >= filter.size() or we've read >= ULLONGS_PER_PAGE
    while (num_bitsets_initialized < filter.size() && numRead < ULLONGS_PER_PAGE) {
        std::bitset<BITSET_SIZE> temp_bitset(pageBuf[numRead]);
        filter[num_bitsets_initialized] = temp_bitset;

        num_bitsets_initialized++;
        numRead++;
    }
}

bool BloomFilter::checkKey(uint64_t key) {
    for (int hash_seed = 0; hash_seed < NUM_HASH_FUNCS; hash_seed++) {
        uint64_t hash_val = XXHash64::hash(&key, sizeof(uint64_t), hash_seed) % total_bits;

        uint64_t bitset_num = hash_val / BITSET_SIZE;
        uint64_t bit_num = hash_val % BITSET_SIZE;

        if (!filter[bitset_num].test(bit_num)) {
            return false;
        }
    }
    return true;
}

std::vector<unsigned long long> BloomFilter::flattenBloomFilter() {
    std::vector<unsigned long long> output;

    for (size_t bitset_num = 0; bitset_num < filter.size(); bitset_num++) {
        output.push_back(filter[bitset_num].to_ullong());
    }

    return output;
}

uint64_t BloomFilter::getTotalBits() { return total_bits; }

uint64_t BloomFilter::getNumPages() { return CEIL_DIV(total_bits, PAGE_SIZE * 8); }
