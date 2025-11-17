#include "../include/bufferpool.hpp"
#include <iostream>
#include <cassert>
#include <filesystem>

#include <fcntl.h>
#include <unistd.h>
#include <cstdint>
#include <vector>
#include <iostream>
#include <sys/stat.h>

class HashMapTester {
public:

    /**
     * Helper function to allocate a page
     */
    static uint64_t* makePage(uint64_t val) {
        uint64_t* p = new uint64_t;
        *p = val;
        return p;
    }

    void test_constructor() {
        HashMap hm(8, 64);

        int expectedBits = ceil(log2(8));
        size_t expectedSize = 1ULL << expectedBits;

        assert(hm.numBitsUsed == expectedBits);
        assert(hm.directory.size() == expectedSize);

        for (size_t i = 0; i < expectedSize; i++) {
            assert(hm.directory[i] != nullptr);
            assert(hm.directory[i]->hashedIndex == i);
            assert(hm.directory[i]->numHashedDigits == expectedBits);
            assert(hm.directory[i]->chainSize == 0);
            assert(hm.directory[i]->first == nullptr);
        }

        std::cout << "Constructor test passed!\n\n" << std::endl;
    }

    void test_insert_and_search() {
        HashMap hm(4, 32);
        uint64_t* p = makePage(10);

        int status = hm.insert("pageA", p, sizeof(uint64_t));
        assert(status == 0);

        auto res = hm.search("pageA");
        assert(res.has_value());
        assert((*res)->pageName == "pageA");
        assert(*((*res)->page) == 10);

        std::cout << "Insert + search test passed!\n\n" << std::endl;
    }

    void test_search_not_found() {
        HashMap hm(4, 32);
        auto r = hm.search("doesNotExist");

        assert(!r.has_value());

        std::cout << "Search missing key test passed!\n\n" << std::endl;
    }

    void test_extend_directory() {
        HashMap hm(2, 16);
        size_t oldSize = hm.directory.size();

        int r = hm.extendDir();
        assert(r == 0);
        assert(hm.directory.size() == oldSize * 2);

        for (size_t i = 0; i < oldSize; i++) {
            assert(hm.directory[i] == hm.directory[i + oldSize]);
        }

        std::cout << "extendDir test passed!\n\n" << std::endl;
    }

    void test_extend_directory_reaches_max() {
        HashMap hm(4, 8); // maxDir = 8

        assert(hm.directory.size() == 4);

        assert(hm.extendDir() == 0);
        assert(hm.directory.size() == 8);

        assert(hm.extendDir() == 1);   // cannot extend further

        std::cout << "extendDir max limit test passed!\n\n" << std::endl;
    }

    void test_insert_trigger_rehash() {

        HashMap hm(2, 64);
        uint64_t* p = makePage(123);

        hm.insert("A", p, 8);
        hm.insert("B", p, 8);
        hm.insert("C", p, 8);
        hm.insert("D", p, 8);
        hm.insert("E", p, 8);
        hm.insert("F", p, 8);
        hm.insert("G", p, 8);
        hm.insert("H", p, 8);
        hm.insert("I", p, 8);
        hm.insert("J", p, 8);
        hm.insert("K", p, 8);
        hm.insert("L", p, 8);


        assert(hm.search("A").has_value());
        assert(hm.search("B").has_value());
        assert(hm.search("C").has_value());
        assert(hm.search("D").has_value());
        assert(hm.search("E").has_value());
        assert(hm.search("F").has_value());
        assert(hm.search("G").has_value());
        assert(hm.search("H").has_value());
        assert(hm.search("I").has_value());
        assert(hm.search("J").has_value());
        assert(hm.search("K").has_value());
        assert(hm.search("L").has_value());

        std::cout << "Rehash-on-overflow test passed!\n\n" << std::endl;
    }

    void test_remove_cases() {
        HashMap hm(2, 32);
        uint64_t* p = makePage(111);

        hm.insert("K1", p, 8);
        hm.insert("K2", p, 8);
        hm.insert("K3", p, 8);

        auto r1 = hm.remove("K1");
        assert(r1.has_value());
        assert((*r1)->pageName == "K1");

        auto r2 = hm.remove("K2");
        assert(r2.has_value());
        assert((*r2)->pageName == "K2");

        auto r3 = hm.remove("K3");
        assert(r3.has_value());
        assert((*r3)->pageName == "K3");

        assert(!hm.search("K1").has_value());

        std::cout << "Remove test passed!\n\n" << std::endl;
    }

    void test_remove_not_found() {
        HashMap hm(4, 32);
        auto r = hm.remove("ghost");

        assert(!r.has_value());

        std::cout << "Remove missing key test passed!\n\n" << std::endl;
    }

};

int main() {

    HashMapTester tester;
    tester.test_constructor();
    tester.test_insert_and_search();
    tester.test_search_not_found();
    tester.test_extend_directory();
    tester.test_extend_directory_reaches_max();
    tester.test_insert_trigger_rehash();
    tester.test_remove_cases();
    tester.test_remove_not_found();

    std::cout << "All HashMap tests passed!\n\n" << std::endl;
    return 0;
}
