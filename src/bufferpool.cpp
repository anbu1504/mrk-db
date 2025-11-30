#include "../include/bufferpool.hpp"

#include <unistd.h>
#include <sys/fcntl.h>

#define SST_PATH(x) ((dbName + "/" + std::to_string(x) + ".sst").c_str())

BufferPool::BufferPool(std::string dbNameVal) : dbName(dbNameVal) {}
BufferPool::~BufferPool() {}

void BufferPool::bread(uint64_t sstNum, uint64_t pageNum, PageBuffer pageBuf, bool bypassCache = false) {
    int fd = open(SST_PATH(sstNum), O_RDONLY);
    pread(fd, pageBuf, PAGE_SIZE, pageNum * PAGE_SIZE);
    close(fd);
}

void BufferPool::bwrite(uint64_t sstNum, uint64_t pageNum, PageBuffer pageBuf) {
    int fd = open(SST_PATH(sstNum), O_RDWR | O_CREAT, 0644);
    pwrite(fd, pageBuf, PAGE_SIZE, pageNum * PAGE_SIZE);
    close(fd);
}

void BufferPool::bdelete(uint64_t sstNum) {
    std::remove(SST_PATH(sstNum));
}

void BufferPool::evictAllPages() {
    return;
}
