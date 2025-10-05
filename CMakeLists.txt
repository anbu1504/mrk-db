cmake_minimum_required(VERSION 4.0)
project(mrk_db)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-std=c++17")

include_directories(include)

add_executable(mrk_db
        external/sample.txt
        include/memtable.h
        include/mrkdb.hpp
        src/memtable.cpp
        src/mrkdb.cpp
        tests/sample.txt
        README.md)
