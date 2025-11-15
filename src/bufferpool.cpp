#include "bufferpool.hpp";

/**
 * @brief Constructor for the BufferPool class
 */

BufferPool::BufferPool(size_t initial, size_t maximal)
    : hashMap(initial) {};

/**
 * @brief Destructor for the BufferPool class
 */
BufferPool::~BufferPool()
{
}

/**
 * @brief Constructor for the HashMap class
 */

HashMap::HashMap(size_t initial)
    : bufferOverflowThreshold(initial),
      numBitsUsed(0),
      directory()
{
}

/**
 * @brief Destructor for the HashMap class
 */

HashMap::~HashMap()
{

}