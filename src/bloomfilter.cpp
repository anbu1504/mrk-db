#include "../include/bloomfilter.hpp"
#include "../external/xxhash64.h"

#define CEIL_DIV(x, y) ((x) / (y) + ((x) % (y) != 0))

/**
 * @brief Constructor for the BloomFilter class
 */
BloomFilter::BloomFilter(ssize_t num_keys)
    : total_bits(num_keys * BITS_PER_ENTRY),
      filter(CEIL_DIV(num_keys * BITS_PER_ENTRY, BITSET_SIZE)) {}

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

    for (int bitset_num = 0; bitset_num < filter.size(); bitset_num++) {
        output.push_back(filter[bitset_num].to_ullong());
    }

    return output;
}