#include <bits/stdc++.h>
#include <random>
#include <filesystem>

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
    std::mt19937_64 rng(42); // fixed seed for reproducibility
    std::vector<uint64_t> sizes = {1000, 10000, 50000, 100000, 300000};

    std::ofstream csv("bin_vs_btree.csv");

    csv << "data_size,structure,throughput_ops_per_sec\n" << std::endl;

    for (uint64_t size : sizes) {
        std::string dbNameBin = "exp_db_bin_" + std::to_string(size);
        std::filesystem::remove_all(dbNameBin);
        
        DB dbBin;
        dbBin.Open(dbNameBin, false); // use binary search

        for (uint64_t i = 0; i < size; i++) {
            dbBin.Put(i, i);
        }

        uint64_t throughputBinary = throughput([&]() {
            for (uint64_t i = 0; i < NUM_OPS; i++) {
                uint64_t key = rng() % size;
                dbBin.Get(key);
            }
        });

        csv << size << ",binary_search," << throughputBinary << "\n" << std::endl;
        dbBin.Close();
        std::filesystem::remove_all(dbNameBin);

        std::string dbNameBTree = "exp_db_btree_" + std::to_string(size);
        std::filesystem::remove_all(dbNameBTree);

        DB dbBTree;
        dbBTree.Open(dbNameBTree, true); // use B-tree search

        uint64_t throughputBTree = throughput([&]() {
            for (uint64_t i = 0; i < NUM_OPS; i++) {
                uint64_t key = rng() % size;
                dbBTree.Get(key);
            }
        });

        csv << size << ",btree_search," << throughputBTree<< "\n" << std::endl;
        dbBTree.Close();
        std::filesystem::remove_all(dbNameBTree);
    }
    csv.close();
}

void putThroughput() {
    std::ofstream csv("put_throughput.csv");

    csv << "data_size,operation,throughput_ops_per_sec\n" << std::endl;

    std::vector<uint64_t> sizes = {50000, 100000, 200000};

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

        csv << size << ",put," << throughputPut << "\n" << std::endl;

        db.Close();
        std::filesystem::remove_all(dbName);
    }
    csv.close();
}

void getThroughput() {
    std::ofstream csv("get_throughput.csv");

    csv << "data_size,operation,throughput_ops_per_sec\n" << std::endl;

    std::mt19937_64 rng(42);
    std::vector<uint64_t> sizes = {50000, 100000, 200000};

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

        csv << size << ",get," << throughputGet << "\n";

        db.Close();
        std::filesystem::remove_all(dbName);
    }
    csv.close();
}

void scanThroughput() {
    std::ofstream csv("scan_throughput.csv");

    csv << "data_size,operation,throughput_ops_per_sec\n" << std::endl;

    std::mt19937_64 rng(42);
    std::vector<uint64_t> sizes = {50000, 100000, 200000};

    for (uint64_t size : sizes) {
        std::string dbName = "exp_scan_" + std::to_string(size);
        std::filesystem::remove_all(dbName);

        DB db;
        db.Open(dbName, true);

        // Preload
        for (uint64_t i = 0; i < size; i++)
            db.Put(i, i);

        uint64_t throughputScan = throughput([&]() {
            for (uint64_t i = 0; i < NUM_OPS; i++) {
                uint64_t a = rng() % (size - 500);
                uint64_t b = a + 500;
                db.Scan(a, b);
            }
        });

        csv << size << ",scan," << throughputScan << "\n" << std::endl;

        db.Close();
        std::filesystem::remove_all(dbName);
    }
    csv.close();
}

int main() {
    binSearchVsBTreeSearch();
    putThroughput();
    getThroughput();
    scanThroughput();
}