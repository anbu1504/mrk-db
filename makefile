# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -Iinclude -g

# Directories
SRC_DIR = src
INC_DIR = include
TEST_DIR = tests
BUILD_DIR = build

# Source files and object files
SRC_FILES = $(wildcard $(SRC_DIR)/*.cpp)
OBJ_FILES = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRC_FILES))

# Test files and executables
TEST_FILES = $(wildcard $(TEST_DIR)/*.cpp)
TEST_EXEC = $(patsubst $(TEST_DIR)/%.cpp, $(BUILD_DIR)/%, $(TEST_FILES))

# Default target
all: $(TEST_EXEC)

# Rule to build test executables
$(BUILD_DIR)/%: $(TEST_DIR)/%.cpp $(OBJ_FILES)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ 

# Rule to build object files from src
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Clean build files
clean:
	rm -rf $(BUILD_DIR)/*.o $(BUILD_DIR)/*

.PHONY: all clean
