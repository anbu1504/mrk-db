# 🏹 MRK-DB V2


### DB
This class basically acts as an API for the user, doesn't do much else.

Holds the following:
- BufferPool
- LSMTree
- dbName
- ~~sstCount~~


### LSMTree
Holds the following:
- Memtable (i.e., the LSMTree's buffer)
- Layers
    - List of SST objects
    - Alternatively can be a list of ints, which are converted to SST objects when needed
- BufferPool* (anyone that needs to read/write should have a reference to bufPool)
- sstCount (might not need this if SST names are determined by level)
    - numLevels

Has these methods:
- Insert, Close (flushes Memtable to disk)
- Get
    - First checks the memtable
    - Then calls SST checkForKey on each level/SST, and iff it returns true, uses SST findPage and fastFwd
- Scan
    - Makes use of SST findPage and fastFwd methods to implement LSM scan (as seen in class)


### BufferPool
Holds the following:
- dbName!!!

Has these methods:
- bread (I just like this name)
    - Accepts: file_id (e.g., 3, 50, temp, etc.), pageNum, pageBuf
    - Could alternatively create an overloaded version that does the same thing, but accepts sstNum and converts it to string first before calling the o.g. bread
    - Note that we should always be reading a page!!
- bwrite
    - The same params as bread
    - It's up to the caller to ensure pageBuf is 0-padded


### SSTView
Holds the following:
- sstNum
- sstMetadata (numKeys,)
- BufferPool* (since it will be reading)
- pageNum (Used for scan)
- pageBuf (Used for scan)
- pagePos (Used for scan)

Has these methods:
- checkForKey(key)
    - Makes use of BloomFilter class
- findPage(key)
    - find the page corresponding to a key, and store it pageBuf
    - store the pageNum in pageNum, set it to MAX_UINT64 if not found
    - Makes use of the BTree class 
- fastFwd(key)
    - fast-forwards the pagePos pointer until it finds the desired key (or wherever the desired key would be)


### SSTWriter
Holds the following:
- new sstNum
- BufferPool* (since it will be writing)

Has these methods:
- writeMiniSST(*memtableData)
    - Used only when flushing the memtable
- mergeSSTs(sstNum1, sstNum2)
    - Creates a BloomFilter
    - Does compaction of the leaf nodes (calls BloomFilter.addKey() as compaction is happening), also writes to tempfile
    - After the above finishes, constructs a BTree using the written tempfile



### BloomFilter
Holds the following:
- BufferPool* (since it will be reading and writing)
- sstNum
- pageOffset
- numKeys (allows you to calculate numBits)

Has these methods:
- checkKey, addKey, addMultipleKeys(*memtableData), ~~writeToSST()~~
- addKey should be called as keys are written during LSMTree compaction
- checkKey should only be called if the SST has been fully created


### BTree
Holds the following:
- BufferPool* (since it will be reading and writing)
- pageOffset
- sstNum
- numKeys (allows you to calculate numInternalNodes)

Has these methods:
- findLeafPage, writeToSST(*memtable) & writeToSST() (either function overload or optional parameter)
- writeToSST should be called after LSMTree compaction, and uses the tempfile if *memtable isn't passed
