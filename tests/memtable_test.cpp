#include "../include/memtable.hpp"
#include <iostream>
#include <cassert>


void test_and_insert_size() {
    Memtable m(10);
    assert(m.getSize() == 0);
    
    std::cout << "Before inserts" << std::endl;
    m.insert(5, 50);
    std::cout << "After first insert" << std::endl;
    m.insert(4, 40);
    m.insert(3, 30);
    assert(m.getSize() == 3);

    std::cout << "Insert pass!" << std::endl;
}



int main() {
    test_and_insert_size();
    return 0;
}