#include <bits/stdc++.h>

#include <filesystem>
#include <random>

#include "../include/mrkdb.hpp"

using namespace std::chrono;

static const uint64_t NUM_OPS = 1000;
#define ONE_MB 1048576 //1MB in bytes
#define ONETWENTYEIGHT_MB_KV 8388608 // 128MB / 16 bytes per KV-pair
#define ONE_GB_KV 625000000 // 1GB / 16 bytes per KV-pair

uint64_t throughput(std::function<void()> fn) {
    auto start = high_resolution_clock::now();
    fn();
    auto end = high_resolution_clock::now();
    double sec = duration<double>(end - start).count();
    return (uint64_t)(NUM_OPS / sec);
}

void scanThroughput() {
    std::cout << "Scan Throughput Experiment Has Started!" << std::endl;
    std::ofstream csv("experiment_results/scan_throughput.csv");

    csv << "data_size,throughput_ops_per_sec\n" << std::endl;

    std::mt19937_64 rng(42);
    std::vector<uint64_t> sizes = {
        1000,    5000,     10000,    50000,     100000,    500000,   1000000,
        5000000, 10000000, 50000000, 100000000, 500000000, 625000000};  // 1GB = 625000000 KV-pairs

    std::string dbName = "exp_scan";
    std::filesystem::remove_all(dbName);
    DB db;
    db.Open(dbName, useBTreeSearch=true);

    for (uint64_t size = ONETWENTYEIGHT_MB_KV; size <= 2 * ONETWENTYEIGHT_MB_KV; size = size + ONETWENTYEIGHT_MB_KV) {
        std::cout << "Starting Size: " << size * 16 / ONE_MB << " MB"  << std::endl;

        // Preload
        for (uint64_t i = size - ONETWENTYEIGHT_MB_KV; i < ONETWENTYEIGHT_MB_KV; i++) db.Put(i, i);

        uint64_t throughputScan = throughput([&]() {
            for (uint64_t i = 0; i < NUM_OPS; i++) {
                uint64_t a = rng() % (size - 500);
                uint64_t b = a + 500;
                db.Scan(a, b);
            }
        });

        csv << size << "," << throughputScan << "\n" << std::endl;

        std::cout << "Size: " << size * 16 / ONE_MB << " MB completed!" << std::endl;

    }
    db.Close();
    std::filesystem::remove_all(dbName);
    csv.close();
    std::ifstream in("experiment_results/scan_throughput.csv");
    std::cout << "\n=== CSV OUTPUT FOR SCAN THROUGHPUT ===\n\n" << std::endl;
    std::cout << in.rdbuf() << std::endl; // dumps entire file directly to stdout
    std::cout << "\n\n" << std::endl;
    std::cout << "=== END CSV OUTPUT FOR SCAN THROUGHPUT ===\n\n";
    std::cout << "Scan Throughput Experiment Has Ended!\n" << std::endl;
    std::cout << "CSV File can also be found at experiment_results/scan_throughput.csv\n" << std::endl;
}

int main() {
    scanThroughput();
    return 0;
}
