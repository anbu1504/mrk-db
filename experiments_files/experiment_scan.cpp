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

static const uint64_t NUM_OPS = 500;
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

void scanThroughput() {
    std::filesystem::create_directory("experiment_results");

    std::cout << "Scan Throughput Experiment Has Started!" << std::endl;
    std::ofstream csv("experiment_results/scan_throughput.csv");

    csv << "data_size,throughput_ops_per_sec\n" << std::endl;

    std::mt19937_64 rng(42);

    std::string dbName = "exp_scan";
    std::filesystem::remove_all(dbName);
    DB db;
    db.Open(dbName, useBTreeSearch=true);

    for (uint64_t size = ONETWENTYEIGHT_MB_KV; size <= 8 * ONETWENTYEIGHT_MB_KV; size = size + ONETWENTYEIGHT_MB_KV) {
        std::cout << "Starting Size: " << size * 16 / ONE_MB << " MB"  << std::endl;

        // Preload
        for (uint64_t i = size - ONETWENTYEIGHT_MB_KV; i < size; i++) db.Put(i, i);

        double throughputScan = throughput([&]() {
            for (uint64_t i = 0; i < NUM_OPS; i++) {
                uint64_t key = rng() % size;
                db.Scan(key, key + 500);
            }
        });

        csv << size << "," << throughputScan << std::endl;

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
