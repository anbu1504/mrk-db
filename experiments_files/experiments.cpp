#include <bits/stdc++.h>

#include <filesystem>
#include <random>

#include "../include/mrkdb.hpp"

using namespace std::chrono;

static const uint64_t NUM_OPS = 50000;

uint64_t throughput(std::function<void()> fn) {
    auto start = high_resolution_clock::now();
    fn();
    auto end = high_resolution_clock::now();
    double sec = duration<double>(end - start).count();
    return (uint64_t)(NUM_OPS / sec);
}

void binSearchVsBTreeSearch() {
    std::cout << "Binary Search vs. B-Tree Search Experiment Has Started!" << std::endl;
    std::mt19937_64 rng(42);  // fixed seed for reproducibility
    // 1000, 5000, 10_000, 50_000, 100_000, 500_000, 1_000_000, 5_000_000, 10_000_000, 50_000_000, 100_000_000,
    // 500_000_000, 625_000_000
    std::vector<uint64_t> sizes = {
        1000,    5000,     10000,    50000,     100000,    500000,   1000000,
        5000000, 10000000, 50000000, 100000000, 500000000, 625000000};  // 1GB = 625000000 KV-pairs

    std::ofstream csv("experiment_results/bin_vs_btree.csv");

    csv << "data_size,bin_search_throughput_ops_per_sec,btree_search_throughput_ops_per_sec\n";

    for (uint64_t size : sizes) {
        std::string dbNameBin = "exp_db_bin_" + std::to_string(size);
        std::filesystem::remove_all(dbNameBin);

        DB dbBin;
        dbBin.Open(dbNameBin, false);  // binary search

        for (uint64_t i = 0; i < size; i++) {
            dbBin.Put(i, i);
        }

        uint64_t throughputBinary = throughput([&]() {
            for (uint64_t i = 0; i < NUM_OPS; i++) {
                uint64_t key = rng() % size;
                dbBin.Get(key);
            }
        });

        dbBin.Close();
        std::filesystem::remove_all(dbNameBin);

        std::string dbNameBTree = "exp_db_btree_" + std::to_string(size);
        std::filesystem::remove_all(dbNameBTree);

        DB dbBTree;
        dbBTree.Open(dbNameBTree, true);  // B-tree search

        for (uint64_t i = 0; i < size; i++) {
            dbBTree.Put(i, i);
        }

        uint64_t throughputBTree = throughput([&]() {
            for (uint64_t i = 0; i < NUM_OPS; i++) {
                uint64_t key = rng() % size;
                dbBTree.Get(key);
            }
        });

        dbBTree.Close();
        std::filesystem::remove_all(dbNameBTree);

        csv << size << "," << throughputBinary << "," << throughputBTree << "\n";
    }

    csv.close();
    std::cout << "Binary Search vs. B-Tree Search Experiment Has Ended!\n" << std::endl;
}

void putThroughput() {
    std::cout << "Put Throughput Experiment Has Started!" << std::endl;
    std::ofstream csv("experiment_results/put_throughput.csv");

    csv << "data_size,throughput_ops_per_sec\n" << std::endl;

    std::vector<uint64_t> sizes = {
        1000,    5000,     10000,    50000,     100000,    500000,   1000000,
        5000000, 10000000, 50000000, 100000000, 500000000, 625000000};  // 1GB = 625000000 KV-pairs

    for (uint64_t size : sizes) {
        std::string dbName = "exp_put_" + std::to_string(size);
        std::filesystem::remove_all(dbName);

        DB db;
        db.Open(dbName, false);  // search method irrelevant for put

        uint64_t throughputPut = throughput([&]() {
            for (uint64_t i = 0; i < NUM_OPS; i++) {
                uint64_t key = i % size;
                db.Put(key, key);
            }
        });

        csv << size << "," << throughputPut << "\n" << std::endl;

        db.Close();
        std::filesystem::remove_all(dbName);
    }
    csv.close();
    std::cout << "Put Throughput Experiment Has Ended!\n" << std::endl;
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
        db.Open(dbName, true);

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
    }
    csv.close();

    std::cout << "Get Throughput Experiment Has Ended!\n" << std::endl;
}

void scanThroughput() {
    std::cout << "Scan Throughput Experiment Has Started!" << std::endl;
    std::ofstream csv("experiment_results/scan_throughput.csv");

    csv << "data_size,throughput_ops_per_sec\n" << std::endl;

    std::mt19937_64 rng(42);
    std::vector<uint64_t> sizes = {
        1000,    5000,     10000,    50000,     100000,    500000,   1000000,
        5000000, 10000000, 50000000, 100000000, 500000000, 625000000};  // 1GB = 625000000 KV-pairs

    for (uint64_t size : sizes) {
        std::string dbName = "exp_scan_" + std::to_string(size);
        std::filesystem::remove_all(dbName);

        DB db;
        db.Open(dbName, true);

        // Preload
        for (uint64_t i = 0; i < size; i++) db.Put(i, i);

        uint64_t throughputScan = throughput([&]() {
            for (uint64_t i = 0; i < NUM_OPS; i++) {
                uint64_t a = rng() % (size - 500);
                uint64_t b = a + 500;
                db.Scan(a, b);
            }
        });

        csv << size << "," << throughputScan << "\n" << std::endl;

        db.Close();
        std::filesystem::remove_all(dbName);
    }
    csv.close();
    std::cout << "Scan Throughput Experiment Has Ended!\n" << std::endl;
}

int main() {
    std::filesystem::remove_all("experiment_results");
    std::filesystem::create_directory("experiment_results");
    binSearchVsBTreeSearch();
    putThroughput();
    getThroughput();
    scanThroughput();
}
