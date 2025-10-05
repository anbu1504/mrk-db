#include "../include/mrkdb.hpp"
#include <filesystem>

int DB::Open(const std::string dbName) {
    bool creation_status = std::filesystem::create_directory(directory_name);
    return 0;
};
