#include <cassert>
#include <iostream>

#include "../include/mrkdb.hpp"

#define MEMTABLE_SST_FILENAME "0.sst"
#define METADATA_FILENAME ".metadata"

class DBTester {
   public:
    void testDBOpen() {
        const std::string testDB = "testdb";

        DB db;
        db.Open(testDB);
        db.Close();

        assert(std::filesystem::exists(testDB));
        assert(std::filesystem::exists(testDB + "/" + METADATA_FILENAME));

        std::cout << "DB::Open() test passed!" << std::endl;

        // Cleanup
        std::filesystem::remove_all(testDB);
    };

    void testDBGetMemtable() {
        const std::string testDB = "testdb";

        std::filesystem::remove_all(testDB);
        std::filesystem::create_directory(testDB);

        DB db;
        db.Open(testDB);

        uint64_t key1 = 1;
        uint64_t val1 = 100;
        uint64_t key2 = 2;
        uint64_t val2 = 200;
        uint64_t key3 = 3;
        uint64_t val3 = 300;

        db.Put(key1, val1);
        db.Put(key2, val2);
        db.Put(key3, val3);

        uint64_t res1 = db.Get(key1).value();
        uint64_t res2 = db.Get(key2).value();
        uint64_t res3 = db.Get(key3).value();

        assert(res1 == 100);
        assert(res2 == 200);
        assert(res3 == 300);

        std::cout << "DB::Get() for memtable test passed!" << std::endl;

        // Cleanup
        std::filesystem::remove_all(testDB);
    }

    void testDBGetSST() {
        const std::string testDB = "testdb";

        std::filesystem::remove_all(testDB);
        std::filesystem::create_directory(testDB);

        DB db;
        db.Open(testDB);

        uint64_t j = 0;
        for (uint64_t i = 0; i < THRESHOLD + 5; i++) {
            db.Put(i, j);
            j++;
        }

        uint64_t val1 = db.Get(1).value();
        uint64_t val2 = db.Get(2).value();
        uint64_t val3 = db.Get(3).value();

        assert(val1 == 1);
        assert(val2 == 2);
        assert(val3 == 3);

        std::cout << "DB::Get() for sst test passed!" << std::endl;

        // Cleanup
        std::filesystem::remove_all(testDB);
    }

    void testDBGetSSTDeeper() {
        const std::string testDB = "testdb";

        std::filesystem::remove_all(testDB);
        std::filesystem::create_directory(testDB);

        DB db;
        db.Open(testDB);

        uint64_t j = 0;
        for (uint64_t i = 0; i < (3 * THRESHOLD) + 5; i++) {
            db.Put(i, j);
            j++;
        }

        uint64_t val1 = db.Get(1).value();
        uint64_t val2 = db.Get(THRESHOLD + 2).value();
        uint64_t val3 = db.Get((2 * THRESHOLD) + 2).value();

        assert(val1 == 1);
        assert(val2 == THRESHOLD + 2);
        assert(val3 == (2 * THRESHOLD) + 2);

        std::cout << "DB::GetSSTDeeper() for sst test deeper passed!" << std::endl;

        // Cleanup
        std::filesystem::remove_all(testDB);
    }

    void testDBGetEmptyMemtable() {
        const std::string testDB = "testdb";

        std::filesystem::remove_all(testDB);
        std::filesystem::create_directory(testDB);

        DB db;
        db.Open(testDB);

        uint64_t key1 = 1;
        uint64_t val1 = 100;
        uint64_t key2 = 2;
        uint64_t val2 = 200;
        uint64_t key3 = 3;
        uint64_t val3 = 300;
        uint64_t key4 = 4;

        db.Put(key1, val1);
        db.Put(key2, val2);
        db.Put(key3, val3);

        std::optional<uint64_t> res1 = db.Get(key4);

        assert(res1 == std::nullopt);

        std::cout << "DB::GetEmpty() for memtable test passed!" << std::endl;

        // Cleanup
        std::filesystem::remove_all(testDB);
    }

    void testDBGetEmptySST() {
        const std::string testDB = "testdb";

        std::filesystem::remove_all(testDB);
        std::filesystem::create_directory(testDB);

        DB db;
        db.Open(testDB);

        uint64_t j = 0;
        for (uint64_t i = 0; i < THRESHOLD + 5; i++) {
            db.Put(i, j);
            j++;
        }

        std::optional<uint64_t> res1 = db.Get(2 * THRESHOLD);

        assert(res1 == std::nullopt);

        std::cout << "DB::GetEmptySST() for sst test passed!" << std::endl;

        // Cleanup
        std::filesystem::remove_all(testDB);
    }

    void testDBGetEmptySSTDeeper() {
        const std::string testDB = "testdb";

        std::filesystem::remove_all(testDB);
        std::filesystem::create_directory(testDB);

        DB db;
        db.Open(testDB);

        uint64_t j = 0;
        for (uint64_t i = 0; i < (3 * THRESHOLD) + 5; i++) {
            db.Put(i, j);
            j++;
        }

        std::optional<uint64_t> res1 = db.Get(5 * THRESHOLD);

        assert(res1 == std::nullopt);

        std::cout << "DB::GetEmptySSTDeeper() for sst test passed!" << std::endl;

        // Cleanup
        std::filesystem::remove_all(testDB);
    }

    void testDBScanMemtable() {
        const std::string testDB = "testdb";

        std::filesystem::remove_all(testDB);
        std::filesystem::create_directory(testDB);

        DB db;
        db.Open(testDB);

        uint64_t key1 = 1;
        uint64_t val1 = 100;
        uint64_t key2 = 2;
        uint64_t val2 = 200;
        uint64_t key3 = 3;
        uint64_t val3 = 300;

        db.Put(key1, val1);
        db.Put(key2, val2);
        db.Put(key3, val3);

        kvPairs res1 = db.Scan(key1, key3);

        assert(res1.size() == 3);

        {
            const auto& [k0, v0] = res1[0];
            const auto& [k1, v1] = res1[1];
            const auto& [k2, v2] = res1[2];

            assert(k0 == key1);
            assert(v0 == val1);

            assert(k1 == key2);
            assert(v1 == val2);

            assert(k2 == key3);
            assert(v2 == val3);
        }

        std::cout << "DB::Scan() for memtable test passed!" << std::endl;

        // Cleanup
        std::filesystem::remove_all(testDB);
    }

    void testDBScanMemtableEmpty() {
        const std::string testDB = "testdb";

        std::filesystem::remove_all(testDB);
        std::filesystem::create_directory(testDB);

        DB db;
        db.Open(testDB);

        uint64_t key1 = 1;
        uint64_t val1 = 100;
        uint64_t key2 = 2;
        uint64_t val2 = 200;
        uint64_t key3 = 3;
        uint64_t val3 = 300;
        uint64_t key4 = 4;
        uint64_t key6 = 6;

        db.Put(key1, val1);
        db.Put(key2, val2);
        db.Put(key3, val3);

        kvPairs res1 = db.Scan(key4, key6);

        assert(res1.empty());

        std::cout << "DB::ScanEmpty() for memtable test passed!" << std::endl;

        // Cleanup
        std::filesystem::remove_all(testDB);
    }

    void testDBScanSST() {
        const std::string testDB = "testdb";

        std::filesystem::remove_all(testDB);
        std::filesystem::create_directory(testDB);

        DB db;
        db.Open(testDB);

        uint64_t j = 0;
        for (uint64_t i = 0; i < THRESHOLD + 5; i++) {
            db.Put(i, j);
            j++;
        }

        uint64_t exp_k0 = 1, exp_v0 = 1;
        uint64_t exp_k1 = 2, exp_v1 = 2;
        uint64_t exp_k2 = 3, exp_v2 = 3;

        kvPairs res1 = db.Scan(1, 3);

        assert(res1.size() == 3);

        {
            const auto& [k0, v0] = res1[0];
            const auto& [k1, v1] = res1[1];
            const auto& [k2, v2] = res1[2];

            assert(k0 == exp_k0);
            assert(v0 == exp_v0);

            assert(k1 == exp_k1);
            assert(v1 == exp_v1);

            assert(k2 == exp_k2);
            assert(v2 == exp_v2);
        }

        std::cout << "DB::Scan() for sst test passed!" << std::endl;

        // Cleanup
        std::filesystem::remove_all(testDB);
    }

    void testDBScanSSTDeeper() {
        const std::string testDB = "testdb";

        std::filesystem::remove_all(testDB);
        std::filesystem::create_directory(testDB);

        DB db;
        db.Open(testDB);

        uint64_t j = 0;
        for (uint64_t i = 0; i < (3 * THRESHOLD) + 5; i++) {
            db.Put(i, j);
            j++;
        }

        uint64_t key1 = 2 * THRESHOLD + 2;
        uint64_t key2 = 2 * THRESHOLD + 20;

        kvPairs res1 = db.Scan(key1, key2);

        assert(res1.size() == 19);

        for (size_t i = 0; i < res1.size(); ++i) {
            const auto& t = res1[i];
            uint64_t k = std::get<0>(t);
            uint64_t v = std::get<1>(t);
            // the notion is that the key and values are the same as per how we inserted it
            // into our database, so we know that it is correct if the key is equal to the value
            // for all returned things in our scan query
            assert(k == v);
        }

        std::cout << "DB::ScanSSTDeeper() for sst deeper test passed!" << std::endl;

        // Cleanup
        std::filesystem::remove_all(testDB);
    }

    void testDBScanSSTEmpty() {
        const std::string testDB = "testdb";

        std::filesystem::remove_all(testDB);
        std::filesystem::create_directory(testDB);

        DB db;
        db.Open(testDB);

        uint64_t j = 0;
        for (uint64_t i = 0; i < (3 * THRESHOLD) + 5; i++) {
            db.Put(i, j);
            j++;
        }

        uint64_t key1 = 4 * THRESHOLD + 2;
        uint64_t key2 = 4 * THRESHOLD + 20;

        kvPairs res1 = db.Scan(key1, key2);

        assert(res1.size() == 0);  // since keys are out of range

        std::cout << "DB::ScanSSTDeeper() for sst empty test passed!" << std::endl;

        // Cleanup
        std::filesystem::remove_all(testDB);
    }

    void testDBScanAcrossSSTs() {
        std::cout << "DB::ScanAcrossSSTs() entered!" << std::endl;
        const std::string testDB = "testdb";

        std::filesystem::remove_all(testDB);
        std::filesystem::create_directory(testDB);

        DB db;
        db.Open(testDB);

        uint64_t j = 0;
        std::cout << "DB::ScanAcrossSSTs() for loop!" << std::endl;
        for (uint64_t i = 0; i < (3 * THRESHOLD) + 5; i++) {
            db.Put(i, j);
            j++;
        }
        std::cout << "DB::ScanAcrossSSTs() after for loop!" << std::endl;

        uint64_t key1 = 0;
        uint64_t key2 = 3 * THRESHOLD;

        std::cout << "DB::ScanAcrossSSTs() scan!" << std::endl;
        kvPairs res1 = db.Scan(key1, key2);
        std::cout << "DB::ScanAcrossSSTs() after scan!" << std::endl;

        assert(res1.size() == (3 * THRESHOLD) + 1);  // size should be 49513

        std::cout << "DB::ScanAcrossSSTs() second for loop!" << std::endl;
        for (size_t i = 0; i < res1.size(); ++i) {
            const auto& t = res1[i];
            uint64_t k = std::get<0>(t);
            uint64_t v = std::get<1>(t);
            // the notion is that the key and values are the same as per how we inserted it
            // into our database, so we know that it is correct if the key is equal to the value
            // for all returned things in our scan query
            assert(k == v);
        }
        std::cout << "DB::ScanAcrossSSTs() outside second for loop!" << std::endl;

        std::cout << "DB::ScanAcrossSSTs() for ssts passed!" << std::endl;

        // Cleanup
        std::filesystem::remove_all(testDB);
    }

    void testDBPut() {
        const std::string testDB = "testdb";

        std::filesystem::remove_all(testDB);
        std::filesystem::create_directory(testDB);

        DB db;
        db.Open(testDB);
        uint64_t key1 = 10;
        uint64_t val1 = 20;
        uint64_t returnValue = db.Put(key1, val1);

        assert(returnValue == 0);

        std::cout << "DB::Put() test passed!" << std::endl;
    }

    void testDBClose() {
        const std::string testDB = "testdb";

        std::filesystem::remove_all(testDB);
        std::filesystem::create_directory(testDB);

        DB db;
        db.Open(testDB);
        int returnValue = db.Close();

        assert(returnValue == 0);

        std::cout << "DB::Close() test passed!" << std::endl;
    }
};

int main() {
    DBTester tester;

    tester.testDBOpen();
    tester.testDBGetMemtable();
    tester.testDBGetSST();
    tester.testDBGetSSTDeeper();
    tester.testDBGetEmptyMemtable();
    tester.testDBGetEmptySST();
    tester.testDBGetEmptySSTDeeper();
    tester.testDBPut();
    tester.testDBPut();
    tester.testDBClose();
    tester.testDBScanMemtable();
    tester.testDBScanMemtableEmpty();
    tester.testDBScanSST();
    tester.testDBScanSSTDeeper();
    tester.testDBScanSSTEmpty();
    tester.testDBScanAcrossSSTs();
    return 0;
}