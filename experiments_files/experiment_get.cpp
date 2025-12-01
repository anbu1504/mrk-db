#include <bits/stdc++.h>

#include <filesystem>
#include <random>

#include "../include/mrkdb.hpp"

using namespace std::chrono;

static const uint64_t NUM_OPS = 10000;

uint64_t throughput(std::function<void()> fn) {
    auto start = high_resolution_clock::now();
    fn();
    auto end = high_resolution_clock::now();
    double sec = duration<double>(end - start).count();
    return (uint64_t)(NUM_OPS / sec);
}

void getThroughput() {
    std::cout << "Get Throughput Experiment Has Started!" << std::endl;

    std::ofstream csv("experiment_results/get_throughput.csv");

    csv << "data_size,throughput_ops_per_sec\n" << std::endl;

    std::mt19937_64 rng(42);
    std::vector<uint64_t> sizes = {
        1000,    5000,     10000,    50000,     100000,    500000,   1000000,
        5000000, 10000000, 50000000, 100000000, 500000000, 625000000};  // 1GB = 625000000 KV-pairs

    for (uint64_t size : sizes) {
        std::string dbName = "exp_get_" + std::to_string(size);
        std::filesystem::remove_all(dbName);

        DB db;
        db.Open(dbName, useBTreeSearch=true);

        // Preload DB
        for (uint64_t i = 0; i < size; i++) {
            db.Put(i, i);
        }

        uint64_t throughputGet = throughput([&]() {
            for (uint64_t i = 0; i < NUM_OPS; i++) {
                uint64_t key = rng() % size;
                db.Get(key);
            }
        });

        csv << size << "," << throughputGet << "\n";

        db.Close();
        std::filesystem::remove_all(dbName);
        std::cout << "Size: " << size << " completed!" << std::endl;
    }
    csv.close();

    std::ifstream in("experiment_results/get_throughput.csv");
    std::cout << "\n=== CSV OUTPUT FOR GET THROUGHPUT ===\n\n" << std::endl;
    std::cout << in.rdbuf() << std::endl; // dumps entire file directly to stdout
    std::cout << "\n\n" << std::endl;
    std::cout << "=== END CSV OUTPUT FOR GET THROUGHPUT ===\n\n";
    std::cout << "Binary Search vs. B-Tree Search Experiment Has Ended!\n" << std::endl;
    std::cout << "Get Throughput Experiment Has Ended!\n" << std::endl;
}

int main() {
    getThroughput();
    return 0;
}