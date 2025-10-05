#ifndef MRKDB_H
#define MRKDB_H

#include <string>

class DB {
public:
    // Open the database 
    int open(const std::string dbName);
}


#endif // MRKDB_H