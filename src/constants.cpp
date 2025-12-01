#include "../include/constants.hpp"

uint64_t binSearch(uint64_t lo, uint64_t hi,
                   const std::function<int(uint64_t)>& cmp) {
    uint64_t result = lo;  // fallback (useful for floor semantics)

    while (lo <= hi) {
        uint64_t mid = lo + (hi - lo) / 2;
        int c = cmp(mid);

        if (c < 0) {
            // key < mid -> move left
            if (mid == 0) break;   // avoid underflow
            hi = mid - 1;
        } else if (c > 0) {
            // key > mid -> move right
            result = mid;          // best known floor position
            lo = mid + 1;
        } else {
            // Found exact match
            return mid;
        }
    }

    return result;  // floor of the search space
}

// number of pages = ceil(total entries / entries in a page)
// if not last page, return entries in a page (entries in a page is actually keys in a page so we have to x2)
// if last page is not full, then return total entries % entries in a page
// if last page is full (i.e. modulo returns 0), then return entries in a page
// pageNum MUST BE >= 1!!
uint64_t calcNumItemsInPage(uint64_t numKeys, uint64_t pageNum) {
    uint64_t currPageNum = pageNum - 1;
    size_t numItems = 2 * numKeys;
    uint64_t numPages = CALC_NUM_PAGES(numKeys * 2, UINT64_SIZE);

    if (currPageNum == numPages - 1) {
        size_t itemsLastPage = numItems % UINT64S_PER_PAGE;
        if (itemsLastPage == 0) {  // 0
            return UINT64S_PER_PAGE;
        } else {
            return itemsLastPage;
        }
    }
    return UINT64S_PER_PAGE;
}
