#pragma once

#include "constants.hpp"
#include "globals.hpp"

/**
 * @class HPage
 * @brief Metadata wrapper for a cached page in the buffer pool.
 */
class HPage {
   public:
    std::string pageID;
    bool dirtyBit;  // Used during eviction
    bool refBit;    // Used by the clock algorithm
    uint64_t probeSeqLen;
    uint64_t* cachedPage;

    /**
     * @brief Constructs an HPage metadata record.
     *
     * @param pageID Identifier of the page
     * @param dirtyBit Whether the page has been modified
     * @param refBit Reference bit for the clock algorithm
     * @param probeSeqLen Probe sequence length used by the robinhood hashing
     * @param cachedPage Pointer to the cached page buffer
     */
    HPage(std::string pageID = "", bool dirtyBit = false, bool refBit = true, uint64_t probeSeqLen = 0,
          uint64_t* cachedPage = nullptr)
        : pageID(pageID), dirtyBit(dirtyBit), refBit(refBit), probeSeqLen(probeSeqLen), cachedPage(cachedPage) {}

    /**
     * @brief Resets the metadata and frees the cached page.
     */
    void reset() {
        pageID.clear();
        dirtyBit = false;
        refBit = true;
        probeSeqLen = 0;
        if (cachedPage) {
            free(cachedPage);
            cachedPage = nullptr;
        }
    }
};

/**
 * @class BufferPool
 * @brief Caches SST pages in memory and coordinates disk I/O.
 */
class BufferPool {
   private:
    std::string dbName;
    uint64_t numCachedPages;
    uint64_t clockHandle;
    std::vector<HPage> hashTable;  // An open-addressing hash table, using Robin Hood hashing

    /**
     * @brief Retrieves a cached page by identifier.
     *
     * @param pageID Identifier of the page to fetch
     */
    HPage* cacheGet(std::string pageID);

    /**
     * @brief Inserts or updates a page in the cache.
     *
     * @param pageID Identifier of the page to cache
     * @param pageBuf Buffer containing page data
     * @param dirty Whether the cached page is dirty
     */
    void cachePut(std::string pageID, PageBuffer pageBuf, bool dirty);

    /**
     * @brief Deletes a cached page entry by index.
     *
     * @param pageIdx Index of the page to remove from the hash table
     */
    bool cacheDel(uint64_t pageIdx);

    /**
     * @brief Evicts a cached page, writing it to disk if dirty.
     *
     * @param victimIdx Index of the page to evict
     */
    bool evict(uint64_t victimIdx);

    /**
     * @brief Runs the clock eviction algorithm when the cache is full.
     */
    void runClockIfFull();

    /**
     * @brief Creates the filesystem path for an SST.
     *
     * @param sstNum SST identifier
     */
    std::string createSSTPath(uint64_t sstNum);

    /**
     * @brief Builds a unique page identifier string.
     *
     * @param sstNum SST identifier
     * @param pageNum Page number within the SST
     */
    std::string createPageID(uint64_t sstNum, uint64_t pageNum);

   public:
    /**
     * @brief Constructs a buffer pool for a database.
     *
     * @param dbNameVal Name of the database directory
     */
    BufferPool(std::string dbNameVal);  // constructor

    /**
     * @brief Destructor that frees cached pages and flushes dirty data.
     */
    ~BufferPool();                      // destructor to free memory

    /**
     * @brief Reads a page from disk or cache.
     *
     * @param sstNum SST identifier
     * @param pageNum Page number to read
     * @param pageBuf Output buffer to populate
     * @param bypassCache If true, read directly from disk
     */
    void bread(uint64_t sstNum, uint64_t pageNum, PageBuffer pageBuf,
               bool bypassCache = false);                                // return number of bytes read

    /**
     * @brief Writes a page to cache and updates the cache entry.
     *
     * @param sstNum SST identifier
     * @param pageNum Page number to write
     * @param pageBuf Buffer containing page data
     */
    void bwrite(uint64_t sstNum, uint64_t pageNum, PageBuffer pageBuf);  // return number of bytes written

    /**
     * @brief Deletes an SST file and evicts its cached pages.
     *
     * @param sstNum SST identifier to delete
     */
    void bdelete(uint64_t sstNum);  // Evicts all pages in bufferpool of given file and deletes the file

    /**
     * @brief Evicts all pages, flushing dirty pages to disk.
     */
    void evictAllPages();           // evicts all pages and writes dirty pages to storage
};
