#include <cassert>
#include <filesystem>
#include <iostream>

#include "../include/mrkdb.hpp"

#define METADATA_FILENAME "meta.sst"
#define THRESHOLD 16384

class DBTester {
   public:
    void testDBOpen() {
        const std::string testDB = "testdb";

        DB db;
        db.Open(testDB);
        db.Close();

        assert(std::filesystem::exists(testDB));
        assert(std::filesystem::exists(testDB + "/" + std::to_string(METADATA_NUM) + ".sst"));

        std::cout << "DB::Open() test passed!" << std::endl;

        // Cleanup
        std::filesystem::remove_all(testDB);
    };

    void testDBGetMemtable() {
        const std::string testDB = "testdb";

        std::filesystem::remove_all(testDB);
        ////std::filesystem::create_directory(testDB);

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

        auto res1 = db.Get(key1);
        auto res2 = db.Get(key2);
        auto res3 = db.Get(key3);

        assert(res1.value() == 100);
        assert(res2.value() == 200);
        assert(res3.value() == 300);

        std::cout << "DB::Get() for memtable test passed!" << std::endl;

        // Cleanup
        std::filesystem::remove_all(testDB);
    }

    void testDBGetSST() {
        const std::string testDB = "testdb";

        std::filesystem::remove_all(testDB);
        // //std::filesystem::create_directory(testDB);

        DB db;
        db.Open(testDB);

        uint64_t j = 0;
        for (uint64_t i = 0; i < THRESHOLD + 5; i++) {
            db.Put(i, j);
            j++;
        }
        // PRINT("done putting");

        auto val1 = db.Get(1);
        auto val2 = db.Get(2);
        auto val3 = db.Get(3);

        // PRINT("done getting");

        assert(val1.value() == 1);
        assert(val2.value() == 2);
        assert(val3.value() == 3);

        std::cout << "DB::Get() for sst test passed!" << std::endl;

        // Cleanup
        std::filesystem::remove_all(testDB);
    }

    void testDBGetSSTDeeper() {
        const std::string testDB = "testdb";

        std::filesystem::remove_all(testDB);
        // std::filesystem::create_directory(testDB);

        DB db;
        db.Open(testDB);

        for (uint64_t i = 0; i < (8 * THRESHOLD) + 5; i++) {
            db.Put(i, i);
        }

        auto val1 = db.Get(1);
        assert(val1.value() == 1);
        auto val2 = db.Get(THRESHOLD + 2);
        auto val3 = db.Get((2 * THRESHOLD) + 2);
        auto val4 = db.Get((3 * THRESHOLD) + 23);
        auto val5 = db.Get((4 * THRESHOLD) + 24);
        auto val6 = db.Get((5 * THRESHOLD) + 30);
        auto val7 = db.Get((6 * THRESHOLD) + 41);
        auto val8 = db.Get((7 * THRESHOLD) + 55);

        assert(val1.value() == 1);
        assert(val2.value() == THRESHOLD + 2);
        assert(val3.value() == (2 * THRESHOLD) + 2);
        assert(val4.value() == (3 * THRESHOLD) + 23);
        assert(val5.value() == (4 * THRESHOLD) + 24);
        assert(val6.value() == (5 * THRESHOLD) + 30);
        assert(val7.value() == (6 * THRESHOLD) + 41);
        assert(val8.value() == (7 * THRESHOLD) + 55);

        std::cout << "DB::GetSSTDeeper() for sst test deeper passed!" << std::endl;

        // Cleanup
        std::filesystem::remove_all(testDB);
    }

    void testDBGetNonExistentMemtable() {
        const std::string testDB = "testdb";

        std::filesystem::remove_all(testDB);
        // std::filesystem::create_directory(testDB);

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

        std::cout << "DB::GetNonExistent() for memtable test passed!" << std::endl;

        // Cleanup
        std::filesystem::remove_all(testDB);
    }

    void testDBGetNonExistentSST() {
        const std::string testDB = "testdb";

        std::filesystem::remove_all(testDB);
        // std::filesystem::create_directory(testDB);

        DB db;
        db.Open(testDB);

        uint64_t j = 0;
        for (uint64_t i = 0; i < THRESHOLD + 5; i++) {
            db.Put(i, j);
            j++;
        }

        std::optional<uint64_t> res1 = db.Get(2 * THRESHOLD);

        assert(res1 == std::nullopt);

        std::cout << "DB::GetNonExistentSST() for sst test passed!" << std::endl;

        // Cleanup
        std::filesystem::remove_all(testDB);
    }

    void testDBGetNonExistentSSTDeeper() {
        const std::string testDB = "testdb";

        std::filesystem::remove_all(testDB);
        // std::filesystem::create_directory(testDB);

        DB db;
        db.Open(testDB);

        uint64_t j = 0;
        for (uint64_t i = 0; i < (3 * THRESHOLD) + 5; i++) {
            db.Put(i, j);
            j++;
        }

        std::optional<uint64_t> res1 = db.Get(5 * THRESHOLD);

        assert(res1 == std::nullopt);

        std::cout << "DB::GetNonExistentSSTDeeper() for sst test passed!" << std::endl;

        // Cleanup
        std::filesystem::remove_all(testDB);
    }

    void testDBScanMemtable() {
        const std::string testDB = "testdb";

        std::filesystem::remove_all(testDB);
        // std::filesystem::create_directory(testDB);

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
        // std::filesystem::create_directory(testDB);

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
        // std::filesystem::create_directory(testDB);

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
        // std::filesystem::create_directory(testDB);

        DB db;
        db.Open(testDB);

        uint64_t j = 0;
        for (uint64_t i = 0; i < (8 * THRESHOLD) + 5; i++) {
            db.Put(i, j);
            j++;
        }

        // 16384
        uint64_t key1 = 2 * THRESHOLD + 2;   // 32770
        uint64_t key2 = 2 * THRESHOLD + 20;  // 32788
        uint64_t key3 = 4 * THRESHOLD + 2;   // 65538
        uint64_t key4 = 6 * THRESHOLD + 20;  // 98324
        uint64_t key5 = 7 * THRESHOLD + 2;   // 114690
        uint64_t key6 = THRESHOLD + 20;      // 16404

        kvPairs res1 = db.Scan(key1, key2);
        // PRINT("after scan 1");
        // assert(res1.size() == 19);

        kvPairs res2 = db.Scan(key1, key3);
        // PRINT("after scan 2");
        kvPairs res3 = db.Scan(key1, key4);
        // PRINT("after scan 3");
        kvPairs res4 = db.Scan(key1, key5);
        // PRINT("after scan 4");
        kvPairs res5 = db.Scan(key6, key1);
        // PRINT("after scan 5");

        assert(res1.size() == 19);
        assert(res2.size() == 32769);
        assert(res3.size() == 65555);
        assert(res4.size() == 81921);
        assert(res5.size() == 16367);

        for (size_t i = 0; i < res1.size(); i++) {
            const auto& t = res1[i];
            uint64_t k = std::get<0>(t);
            uint64_t v = std::get<1>(t);
            // the notion is that the key and values are the same as per how we inserted it
            // into our database, so we know that it is correct if the key is equal to the value
            // for all returned things in our scan query
            assert(k == v);
        }

        for (size_t i = 0; i < res2.size(); i++) {
            const auto& t = res2[i];
            uint64_t k = std::get<0>(t);
            uint64_t v = std::get<1>(t);
            // the notion is that the key and values are the same as per how we inserted it
            // into our database, so we know that it is correct if the key is equal to the value
            // for all returned things in our scan query
            assert(k == v);
        }

        for (size_t i = 0; i < res3.size(); i++) {
            const auto& t = res3[i];
            uint64_t k = std::get<0>(t);
            uint64_t v = std::get<1>(t);
            // the notion is that the key and values are the same as per how we inserted it
            // into our database, so we know that it is correct if the key is equal to the value
            // for all returned things in our scan query
            assert(k == v);
        }

        for (size_t i = 0; i < res4.size(); i++) {
            const auto& t = res4[i];
            uint64_t k = std::get<0>(t);
            uint64_t v = std::get<1>(t);
            // the notion is that the key and values are the same as per how we inserted it
            // into our database, so we know that it is correct if the key is equal to the value
            // for all returned things in our scan query
            assert(k == v);
        }

        for (size_t i = 0; i < res5.size(); i++) {
            const auto& t = res5[i];
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
        // std::filesystem::create_directory(testDB);

        DB db;
        db.Open(testDB);

        uint64_t missingNumber = 267;
        for (uint64_t i = 0; i < (3 * THRESHOLD) + 5; i++) {
            if (i != missingNumber) {
                db.Put(i, i);
            }
        }

        uint64_t key1 = 4 * THRESHOLD + 2;
        uint64_t key2 = 4 * THRESHOLD + 20;
        uint64_t key3 = 0;
        uint64_t key4 = 270;

        kvPairs res1 = db.Scan(key1, key2);
        kvPairs res2 = db.Scan(key3, key4);

        assert(res1.size() == 0);  // since keys are out of range
        assert(res2.size() == 270);

        bool found = false;
        for (size_t i = 0; i < res2.size(); i++) {
            const auto& t = res2[i];
            uint64_t k = std::get<0>(t);
            uint64_t v = std::get<1>(t);

            if (std::get<0>(t) == 267) {
                found = true;
            }

            assert(k == v);
        }
        assert(!found);  // i.e. the number that was not added was not found in the resulting scan

        std::cout << "DB::ScanSSTDeeper() for sst empty test passed!" << std::endl;

        // Cleanup
        std::filesystem::remove_all(testDB);
    }

    void testDBScanAcrossSSTs() {
        const std::string testDB = "testdb";

        std::filesystem::remove_all(testDB);
        // std::filesystem::create_directory(testDB);

        DB db;
        db.Open(testDB);

        uint64_t j = 0;
        for (uint64_t i = 0; i < (3 * THRESHOLD) + 5; i++) {
            db.Put(i, j);
            j++;
        }

        uint64_t key1 = 0;
        uint64_t key2 = 3 * THRESHOLD;

        kvPairs res1 = db.Scan(key1, key2);

        assert(res1.size() == (3 * THRESHOLD) + 1);  // size should be 49513

        for (size_t i = 0; i < res1.size(); i++) {
            const auto& t = res1[i];
            uint64_t k = std::get<0>(t);
            uint64_t v = std::get<1>(t);
            // the notion is that the key and values are the same as per how we inserted it
            // into our database, so we know that it is correct if the key is equal to the value
            // for all returned things in our scan query
            assert(k == v);
        }

        std::cout << "DB::ScanAcrossSSTs() for ssts passed!" << std::endl;

        // Cleanup
        std::filesystem::remove_all(testDB);
    }

    void testDBPut() {
        const std::string testDB = "testdb";

        std::filesystem::remove_all(testDB);
        // std::filesystem::create_directory(testDB);

        DB db;
        db.Open(testDB);
        uint64_t key1 = 10;
        uint64_t val1 = 20;
        uint64_t returnValue = db.Put(key1, val1);

        assert(returnValue == 0);

        std::cout << "DB::Put() test passed!" << std::endl;
    }

    void testDBCloseFully() {
        const std::string testDB = "testdb";

        std::filesystem::remove_all(testDB);
        // std::filesystem::create_directory(testDB);

        DB db;
        db.Open(testDB);
        int returnValue = db.Close();

        assert(returnValue == 0);

        std::cout << "DB::CloseFully() test passed!" << std::endl;
    }

    void testDBReopenGet() {
        const std::string testDB = "testdb";
        std::filesystem::remove_all(testDB);
        // std::filesystem::create_directory(testDB);

        DB db;
        db.Open(testDB);

        // Insert enough to flush to SST
        uint64_t j = 0;

        for (uint64_t i = 0; i < (3 * THRESHOLD) + 5; i++) {
            db.Put(i, j);
            j++;
        }

        db.Close();
        DB db2;
        db2.Open(testDB);

        // Make sure data is still there
        assert(db2.Get(1).value() == 1);
        assert(db2.Get(2 * THRESHOLD + 51).value() == 2 * THRESHOLD + 51);
        assert(db2.Get(THRESHOLD + 2213).value() == THRESHOLD + 2213);

        db2.Close();
        std::filesystem::remove_all(testDB);
        std::cout << "DB::testDBReopenGet() persistance test passed!" << std::endl;
    }

    void testDBScanGetMultipleKeysNotThere() {
        const std::string testDB = "testdb";
        std::filesystem::remove_all(testDB);
        // std::filesystem::create_directory(testDB);

        DB db;
        db.Open(testDB);

        for (uint64_t i = 0; i < (3 * THRESHOLD) + 5; i++) {
            if (i % 2 == 0) {
                db.Put(i, i);
            }
        }

        uint64_t key1 = 0;
        uint64_t key2 = 2 * THRESHOLD;

        kvPairs res1 = db.Scan(key1, key2);

        assert(res1.size() == THRESHOLD + 1);

        bool oddFound = false;

        for (size_t i = 0; i < res1.size(); i++) {
            const auto& t = res1[i];
            uint64_t k = std::get<0>(t);
            uint64_t v = std::get<1>(t);

            if (std::get<0>(t) % 2 != 0) {
                oddFound = true;  // since we didn't insert any odd key value pairs
            }

            // the notion is that the key and values are the same as per how we inserted it
            // into our database, so we know that it is correct if the key is equal to the value
            // for all returned things in our scan query
            assert(k == v);
        }
        assert(!oddFound);  // i.e. ensuring that no odd key value pairs were found
        std::cout << "DB::testDBScanGetMultipleKeysNotThere() test passed!" << std::endl;
    }

    void testDBDeleteMemtable() {
        const std::string testDB = "testdb";

        std::filesystem::remove_all(testDB);
        // std::filesystem::create_directory(testDB);

        DB db;
        db.Open(testDB);

        db.Put(10, 100);
        db.Delete(10);

        auto res = db.Get(10);
        assert(res == std::nullopt);

        std::cout << "DB::testDBDeleteMemtable() test passed!" << std::endl;
    }

    void testDBDeleteSST() {
        const std::string testDB = "testdb";

        std::filesystem::remove_all(testDB);
        // std::filesystem::create_directory(testDB);

        DB db;
        db.Open(testDB);

        for (uint64_t i = 0; i < (8 * THRESHOLD) + 5; i++) {
            db.Put(i, i);
        }

        db.Delete((6 * THRESHOLD) + 67);

        auto res = db.Get((6 * THRESHOLD) + 67);

        assert(res == std::nullopt);
        std::cout << "DB::testDBDeleteSST() test passed!" << std::endl;
    }

    void testDBDeletesTwice() {
        const std::string testDB = "testdb";

        std::filesystem::remove_all(testDB);
        // std::filesystem::create_directory(testDB);

        DB db;
        db.Open(testDB);

        for (uint64_t i = 0; i < (8 * THRESHOLD) + 5; i++) {
            db.Put(i, i);
        }

        db.Delete((7 * THRESHOLD) + 76);
        db.Delete((7 * THRESHOLD) + 76);

        auto res = db.Get((7 * THRESHOLD) + 76);

        assert(res == std::nullopt);
        std::cout << "DB::testDBDeletesTwice() test passed!" << std::endl;
    }

    void testDBDeleteReinsert() {
        const std::string testDB = "testdb";

        std::filesystem::remove_all(testDB);
        // std::filesystem::create_directory(testDB);

        DB db;
        db.Open(testDB);

        db.Put(5, 500);
        db.Delete(5);
        db.Put(5, 999);

        auto res = db.Get(5);

        assert(res.value() == 999);
        std::cout << "DB::testDBDeleteReinsert() test passed!" << std::endl;
    }

    void testDBMultipleDeletes() {
        const std::string testDB = "testdb";

        std::filesystem::remove_all(testDB);
        // std::filesystem::create_directory(testDB);

        DB db;
        db.Open(testDB);

        for (uint64_t i = 0; i < (8 * THRESHOLD) + 5; i++) {
            db.Put(i, i);
        }

        db.Delete((4 * THRESHOLD) + 23);
        db.Delete((5 * THRESHOLD) + 999);
        db.Delete((6 * THRESHOLD) + 67);
        db.Delete((7 * THRESHOLD) + 6768);

        auto res1 = db.Get((4 * THRESHOLD) + 23);
        auto res2 = db.Get((5 * THRESHOLD) + 999);
        auto res3 = db.Get((6 * THRESHOLD) + 67);
        auto res4 = db.Get((7 * THRESHOLD) + 6768);

        assert(res1 == std::nullopt);
        assert(res2 == std::nullopt);
        assert(res3 == std::nullopt);
        assert(res4 == std::nullopt);
        std::cout << "DB::testDBMultipleDeletes() test passed!" << std::endl;
    }

    void testDBMultipleDeletesDeeper() {
        const std::string testDB = "testdb";

        std::filesystem::remove_all(testDB);
        // std::filesystem::create_directory(testDB);

        DB db;
        db.Open(testDB);

        for (uint64_t i = 0; i < (8 * THRESHOLD) + 5; i++) {
            db.Put(i, i);
        }

        db.Delete((4 * THRESHOLD) + 23);
        db.Delete((5 * THRESHOLD) + 999);
        db.Delete((6 * THRESHOLD) + 67);
        db.Delete((7 * THRESHOLD) + 6768);

        for (uint64_t i = 9 * THRESHOLD; i < (18 * THRESHOLD) + 5; i++) {
            db.Put(i, i);
        }

        auto res1 = db.Get((4 * THRESHOLD) + 23);
        auto res2 = db.Get((5 * THRESHOLD) + 999);
        auto res3 = db.Get((6 * THRESHOLD) + 67);
        auto res4 = db.Get((7 * THRESHOLD) + 6768);

        assert(res1 == std::nullopt);
        assert(res2 == std::nullopt);
        assert(res3 == std::nullopt);
        assert(res4 == std::nullopt);
        std::cout << "DB::testDBMultipleDeletesDeeper() test passed!" << std::endl;
    }

    void testDBPersistentDeletes() {
        const std::string testDB = "testdb";

        std::filesystem::remove_all(testDB);

        DB db;
        db.Open(testDB);

        for (uint64_t i = 0; i < (8 * THRESHOLD) + 5; i++) {
            db.Put(i, i);
        }
        auto resv1 = db.Get((4 * THRESHOLD) + 23);
        auto resv2 = db.Get((5 * THRESHOLD) + 999);
        auto resv3 = db.Get((6 * THRESHOLD) + 67);
        auto resv4 = db.Get((7 * THRESHOLD) + 6768);

        assert((resv1.value() = (4 * THRESHOLD) + 23));
        assert((resv2.value() = (5 * THRESHOLD) + 999));
        assert((resv3.value() = (6 * THRESHOLD) + 67));
        assert((resv4.value() = (7 * THRESHOLD) + 6768));

        db.Delete((4 * THRESHOLD) + 23);
        db.Delete((5 * THRESHOLD) + 999);
        db.Delete((6 * THRESHOLD) + 67);
        db.Delete((7 * THRESHOLD) + 6768);

        db.Close();
        DB db2;
        db2.Open(testDB);

        auto res1 = db2.Get((4 * THRESHOLD) + 23);
        auto res2 = db2.Get((5 * THRESHOLD) + 999);
        auto res3 = db2.Get((6 * THRESHOLD) + 67);
        auto res4 = db2.Get((7 * THRESHOLD) + 6768);

        assert(res1 == std::nullopt);
        assert(res2 == std::nullopt);
        assert(res3 == std::nullopt);
        assert(res4 == std::nullopt);

        std::cout << "DB::testDBPersistentDeletes() test passed!" << std::endl;
    }

    void testDBUpdatesMemtable() {
        const std::string testDB = "testdb";

        std::filesystem::remove_all(testDB);

        DB db;
        db.Open(testDB);

        db.Put(10, 100);
        db.Put(10, 200);

        auto res = db.Get(10);
        assert(res == 200);

        std::cout << "DB::testDBUpdateMemtable() test passed!" << std::endl;
    }

    void testDBUpdatesTwice() {
        const std::string testDB = "testdb";

        std::filesystem::remove_all(testDB);
        // std::filesystem::create_directory(testDB);

        DB db;
        db.Open(testDB);

        for (uint64_t i = 0; i < (8 * THRESHOLD) + 5; i++) {
            db.Put(i, i);
        }

        db.Put((7 * THRESHOLD) + 76, 2);
        db.Put((7 * THRESHOLD) + 76, 44);

        auto res = db.Get((7 * THRESHOLD) + 76);

        assert(res == 44);
        std::cout << "DB::testDBUpdatesTwice() test passed!" << std::endl;
    }

    void testDBMultipleUpdates() {
        const std::string testDB = "testdb";

        std::filesystem::remove_all(testDB);
        // std::filesystem::create_directory(testDB);

        DB db;
        db.Open(testDB);

        for (uint64_t i = 0; i < (8 * THRESHOLD) + 5; i++) {
            db.Put(i, i);
        }

        db.Put((4 * THRESHOLD) + 23, 33);
        db.Put((5 * THRESHOLD) + 999, 44);
        db.Put((6 * THRESHOLD) + 67, 55);
        db.Put((7 * THRESHOLD) + 6768, 66);

        auto res1 = db.Get((4 * THRESHOLD) + 23);
        auto res2 = db.Get((5 * THRESHOLD) + 999);
        auto res3 = db.Get((6 * THRESHOLD) + 67);
        auto res4 = db.Get((7 * THRESHOLD) + 6768);

        assert(res1.value() == 33);
        assert(res2.value() == 44);
        assert(res3.value() == 55);
        assert(res4.value() == 66);
        std::cout << "DB::testDBMultipleUpdates() test passed!" << std::endl;
    }

    void testDBMultipleUpdatesDeeper() {
        const std::string testDB = "testdb";

        std::filesystem::remove_all(testDB);
        // std::filesystem::create_directory(testDB);

        DB db;
        db.Open(testDB);

        for (uint64_t i = 0; i < (8 * THRESHOLD) + 5; i++) {
            db.Put(i, i);
        }

        db.Put((4 * THRESHOLD) + 23, 33);
        db.Put((5 * THRESHOLD) + 999, 44);
        db.Put((6 * THRESHOLD) + 67, 55);
        db.Put((7 * THRESHOLD) + 6768, 66);

        for (uint64_t i = 9 * THRESHOLD; i < (18 * THRESHOLD) + 5; i++) {
            db.Put(i, i);
        }

        auto res1 = db.Get((4 * THRESHOLD) + 23);
        auto res2 = db.Get((5 * THRESHOLD) + 999);
        auto res3 = db.Get((6 * THRESHOLD) + 67);
        auto res4 = db.Get((7 * THRESHOLD) + 6768);

        assert(res1.value() == 33);
        assert(res2.value() == 44);
        assert(res3.value() == 55);
        assert(res4.value() == 66);
        std::cout << "DB::testDBMultipleUpdatesDeeper() test passed!" << std::endl;
    }

    void testDBPersistentUpdates() {
        const std::string testDB = "testdb";

        std::filesystem::remove_all(testDB);

        DB db;
        db.Open(testDB);

        for (uint64_t i = 0; i < (8 * THRESHOLD) + 5; i++) {
            db.Put(i, i);
        }
        auto resv1 = db.Get((4 * THRESHOLD) + 23);
        auto resv2 = db.Get((5 * THRESHOLD) + 999);
        auto resv3 = db.Get((6 * THRESHOLD) + 67);
        auto resv4 = db.Get((7 * THRESHOLD) + 6768);

        assert((resv1.value() = (4 * THRESHOLD) + 23));
        assert((resv2.value() = (5 * THRESHOLD) + 999));
        assert((resv3.value() = (6 * THRESHOLD) + 67));
        assert((resv4.value() = (7 * THRESHOLD) + 6768));

        db.Put((4 * THRESHOLD) + 23, 44);
        db.Put((5 * THRESHOLD) + 999, 55);
        db.Put((6 * THRESHOLD) + 67, 42);
        db.Put((7 * THRESHOLD) + 6768, 67);

        db.Close();
        DB db2;
        db2.Open(testDB);
        db2.Put((7 * THRESHOLD) + 6768, 68);

        auto res1 = db2.Get((4 * THRESHOLD) + 23);
        auto res2 = db2.Get((5 * THRESHOLD) + 999);
        auto res3 = db2.Get((6 * THRESHOLD) + 67);
        auto res4 = db2.Get((7 * THRESHOLD) + 6768);

        assert(res1.value() == 44);
        assert(res2.value() == 55);
        assert(res3.value() == 42);
        assert(res4.value() == 68);

        std::cout << "DB::testDBPersistentUpdates() test passed!" << std::endl;
    }

    void testDBDeleteScans() {
        const std::string testDB = "testdb";
        std::filesystem::remove_all(testDB);
        // std::filesystem::create_directory(testDB);

        DB db;
        db.Open(testDB);

        for (uint64_t i = 0; i < (3 * THRESHOLD) + 5; i++) {
            db.Put(i, i);
        }

        for (uint64_t i = 0; i < (3 * THRESHOLD) + 5; i++) {
            if (i % 2 == 1) {
                db.Delete(i);
            }
        }

        uint64_t key1 = 0;
        uint64_t key2 = 2 * THRESHOLD;

        kvPairs res1 = db.Scan(key1, key2);

        assert(res1.size() == THRESHOLD + 1);

        bool oddFound = false;

        for (size_t i = 0; i < res1.size(); i++) {
            const auto& t = res1[i];
            uint64_t k = std::get<0>(t);
            uint64_t v = std::get<1>(t);

            if (std::get<0>(t) % 2 != 0) {
                oddFound = true;  // since we didn't insert any odd key value pairs
            }

            // the notion is that the key and values are the same as per how we inserted it
            // into our database, so we know that it is correct if the key is equal to the value
            // for all returned things in our scan query
            assert(k == v);
        }
        assert(!oddFound);  // i.e. ensuring that no odd key value pairs were found
        std::cout << "DB::testDBDeleteScans() test passed!" << std::endl;
    }
};

int main() {
    DBTester tester;

    tester.testDBOpen();
    tester.testDBGetMemtable();
    tester.testDBGetSST();
    tester.testDBGetSSTDeeper();
    tester.testDBGetNonExistentMemtable();
    tester.testDBGetNonExistentSST();
    tester.testDBGetNonExistentSSTDeeper();
    tester.testDBPut();
    tester.testDBCloseFully();
    tester.testDBScanMemtable();
    tester.testDBScanMemtableEmpty();
    tester.testDBScanSST();
    tester.testDBScanSSTDeeper();
    tester.testDBScanSSTEmpty();
    tester.testDBScanAcrossSSTs();
    tester.testDBReopenGet();
    tester.testDBScanGetMultipleKeysNotThere();
    tester.testDBDeleteMemtable();
    tester.testDBDeleteSST();
    tester.testDBDeletesTwice();
    tester.testDBDeleteReinsert();
    tester.testDBMultipleDeletes();
    tester.testDBMultipleDeletesDeeper();
    tester.testDBPersistentDeletes();
    tester.testDBUpdatesMemtable();
    tester.testDBUpdatesTwice();
    tester.testDBMultipleUpdates();
    tester.testDBMultipleUpdatesDeeper();
    tester.testDBPersistentUpdates();
    tester.testDBDeleteScans();
    return 0;
}
