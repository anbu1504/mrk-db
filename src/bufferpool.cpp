#include "../include/bufferpool.hpp"

#include <unistd.h>
#include <sys/fcntl.h>
#include <cstring>

#define SST_PATH(x) ((dbName + "/" + std::to_string(x) + ".sst").c_str())

BufferPool::BufferPool(std::string dbNameVal) : dbName(dbNameVal) {}
BufferPool::~BufferPool() {}

void BufferPool::bread(uint64_t sstNum, uint64_t pageNum, PageBuffer pageBuf, bool bypassCache) {
    uint64_t* cachedPage = hashMap->get(sstNum + "_" + pageNum);
    if (cachedPage) {
        memcpy(pageBuf, cachedPage, PAGE_SIZE);
    } else {
        int fd = open(SST_PATH(sstNum), O_RDONLY);
        pread(fd, pageBuf, PAGE_SIZE, pageNum * PAGE_SIZE);
        close(fd);
    }
}

void BufferPool::bwrite(uint64_t sstNum, uint64_t pageNum, PageBuffer pageBuf) {
    hashMap->put(sstNum + "_" + pageNum, pageBuf);
    // int fd = open(SST_PATH(sstNum), O_RDWR | O_CREAT, 0644);
    // pwrite(fd, pageBuf, PAGE_SIZE, pageNum * PAGE_SIZE);
    // close(fd);
}

void BufferPool::bdelete(uint64_t sstNum) {
    std::remove(SST_PATH(sstNum));
    hashMap->deleteAllWithPrefix(std::to_string(sstNum));
}

void BufferPool::evictAllPages() {
    hashMap->evictAll();
}
