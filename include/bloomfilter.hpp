#include <vector>
#include <bitset>

#define PAGE_SIZE 4096
#define BITSET_SIZE 64 // size of unsigned long long

#define BITS_PER_ENTRY 12
#define NUM_HASH_FUNCS 8

class BloomFilter {

private:
    uint64_t total_bits;
    std::vector<std::bitset<BITSET_SIZE>> filter;
    size_t num_bitsets_initialized;
    
public:
    BloomFilter(size_t num_keys);
    BloomFilter(uint64_t total_bits);
    ~BloomFilter();

    uint64_t getTotalBits();
    void addKey(uint64_t key);
    void initFromBuf(unsigned long long pageBuf[PAGE_SIZE / sizeof(unsigned long long)]);
    bool checkKey(uint64_t key);
    std::vector<unsigned long long> flattenBloomFilter();
    void printish();
};