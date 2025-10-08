#include "../include/mrkdb.hpp"
#include <iostream>
#include <cassert>

#define MEMTABLE_SST_FILENAME "0.sst"
#define METADATA_FILENAME ".metadata"

class DBTester
{
public:
    void testDBOpen()
    {
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

        db.Put(1, 100);
        db.Put(2, 200);
        db.Put(3, 300);

        uint64_t val1 = db.Get(1);
        uint64_t val2 = db.Get(2);
        uint64_t val3 = db.Get(3);

        assert(val1 == 100);
        assert(val2 == 200);
        assert(val3 == 300);

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

        int j = 0;
        for (int i = 0; i < THRESHOLD + 5; i++) {
            db.Put(i, j);
            j++;
        }

        uint64_t val1 = db.Get(1);
        uint64_t val2 = db.Get(2);
        uint64_t val3 = db.Get(3);

        assert(val1 == 1);
        assert(val2 == 2);
        assert(val3 == 3);

        std::cout << "DB::Get() for sst test passed!" << std::endl;
        
        // Cleanup
        std::filesystem::remove_all(testDB);
    }

    void testDBPut() {
        const std::string testDB = "testdb";

        std::filesystem::remove_all(testDB);
        std::filesystem::create_directory(testDB);

        DB db;
        db.Open(testDB);
        int returnValue = db.Put(10, 20);

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
    tester.testDBPut();
    tester.testDBPut();
    tester.testDBClose();
    return 0;
}