#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <functional>
#include <fstream>
#include <cstdint>
#include <filesystem>

#include <filesystem>
#include <random>

#include "../include/mrkdb.hpp"

using namespace std::chrono;

static const uint64_t NUM_OPS = 100;
#define ONE_MB 1048576 //1MB in bytes
#define ONETWENTYEIGHT_MB_KV 8388608 // 128MB / 16 bytes per KV-pair
#define ONE_GB_KV 625000000 // 1GB / 16 bytes per KV-pair

double throughput(std::function<void()> fn) {
    auto start = high_resolution_clock::now();
    fn();
    auto end = high_resolution_clock::now();
    double sec = duration<double>(end - start).count();
    return (NUM_OPS / sec);
}

void binSearchVsBTreeSearch() {
    std::cout << "Binary Search vs. B-Tree Search Experiment Has Started!" << std::endl;
    std::mt19937_64 rng(42);  // fixed seed for reproducibility
    // 1000, 5000, 10_000, 50_000, 100_000, 500_000, 1_000_000, 5_000_000, 10_000_000, 50_000_000, 100_000_000,
    // 500_000_000, 625_000_000

    std::ofstream csv("experiment_results/bin_vs_btree.csv");

    csv << "data_size,bin_search_throughput_ops_per_sec,btree_search_throughput_ops_per_sec\n";

    std::string dbNameBin = "exp_db_bin";
    std::filesystem::remove_all(dbNameBin);
    DB dbBin;
    dbBin.Open(dbNameBin, useBTreeSearch=false);  // binary search

    std::string dbNameBTree = "exp_db_btree";
    std::filesystem::remove_all(dbNameBTree);

    DB dbBTree;
    dbBTree.Open(dbNameBTree, useBTreeSearch=true);  // B-tree search

    for (uint64_t size = ONETWENTYEIGHT_MB_KV; size <= 8 * ONETWENTYEIGHT_MB_KV; size = size + ONETWENTYEIGHT_MB_KV) {
        std::cout << "Starting Size: " << size * 16 / ONE_MB << " MB"  << std::endl;

        rng.seed(size);
        for (uint64_t i = size - ONETWENTYEIGHT_MB_KV; i < size; i++) {
            dbBin.Put(i, i);
        }
        double throughputBinary = throughput([&]() {
            for (uint64_t i = 0; i < NUM_OPS; i++) {
                if (rng() % 2 == 0) {
                    uint64_t key1 = rng() % size;
                    uint64_t key2 = rng() % size;
                    if (key1 > key2) std::swap(key1, key2);
                    dbBin.Scan(key1, key2 + 1);
                } else {
                    uint64_t key = rng() % size;
                    dbBin.Get(key);
                }
            }
        });

        rng.seed(size);
        for (uint64_t i = size - ONETWENTYEIGHT_MB_KV; i < size; i++) {
            dbBTree.Put(i, i);
        }

        double throughputBTree = throughput([&]() {
           for (uint64_t i = 0; i < NUM_OPS; i++) {
                if (rng() % 2 == 0) {
                    uint64_t key1 = rng() % size;
                    uint64_t key2 = rng() % size;
                    if (key1 > key2) std::swap(key1, key2);
                    dbBTree.Scan(key1, key2 + 1);
                } else {
                    uint64_t key = rng() % size;
                    dbBTree.Get(key);
                }
            }
        });

        csv << size << "," << throughputBinary << "," << throughputBTree << std::endl;
        std::cout << "Size: " << size * 16 / ONE_MB << " MB completed!" << std::endl;
    }

    dbBin.Close();
    std::filesystem::remove_all(dbNameBin);
    dbBTree.Close();
    std::filesystem::remove_all(dbNameBTree);

    csv.close();
    std::ifstream in("experiment_results/bin_vs_btree.csv");
    std::cout << "\n=== CSV OUTPUT FOR BINARY SEARCH VS. BTREE ===\n\n" << std::endl;
    std::cout << in.rdbuf() << std::endl; // dumps entire file directly to stdout
    std::cout << "\n\n" << std::endl;
    std::cout << "=== END CSV OUTPUT FOR BINARY SEARCH VS. BTREE ===\n\n";
    std::cout << "Binary Search vs. B-Tree Search Experiment Has Ended!\n" << std::endl;
    std::cout << "CSV File can also be found at experiment_results/bin_vs_btree.csv\n" << std::endl;
}

int main() {
    binSearchVsBTreeSearch();
    return 0;
}
