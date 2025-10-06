#include "../include/memtable.hpp"
#include <iostream>
#include <cassert>


void test_and_insert_size() {
    Memtable m(10);
    assert(m.getSize() == 0);
    m.insert(5, 50);
    m.insert(4, 40);
    m.insert(3, 30);
    assert(m.getSize() == 3);

    std::cout << "Insert pass!" << std::endl;
}

void test_height_and_balance() {
    Memtable m2(10);
    m2.insert(10, 1);
    m2.insert(5, 2);
    m2.insert(15, 3);
}



int main() {
    test_and_insert_size();
    return 0;
}