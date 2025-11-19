#include <vector>
#include <bitset>

#define BITSET_SIZE 64 // size of unsigned long long

#define BITS_PER_ENTRY 12
#define NUM_HASH_FUNCS 8

class BloomFilter {

private:
    uint64_t total_bits;
    std::vector<std::bitset<BITSET_SIZE>> filter;
    
public:
    BloomFilter(ssize_t num_keys);
    ~BloomFilter();

    void addKey(uint64_t key);
    bool checkKey(uint64_t key);
    std::vector<unsigned long long> flattenBloomFilter();
};