CXX = g++
CXXFLAGS = -Wall -Wextra -g -std=c++17
LDFLAGS = 
EXECUTABLE = mrkdb_tests

BUILD_DIR = build
SRC_DIR = src
TEST_DIR = tests
TEST_DB = testdb
INCLUDE_DIR = include

LIB_SOURCES = $(wildcard $(SRC_DIR)/*.cpp)

TEST_MAIN = $(TEST_DIR)/mrkdb_tests.cpp

LIB_OBJECTS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(LIB_SOURCES))

TEST_OBJECT = $(patsubst $(TEST_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(TEST_MAIN))

OBJECTS = $(LIB_OBJECTS) $(TEST_OBJECT)

all: $(BUILD_DIR) $(EXECUTABLE)

$(BUILD_DIR):
	mkdir -p $@

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)
	
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

$(BUILD_DIR)/mrkdb_tests.o: $(TEST_DIR)/mrkdb_tests.cpp
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

.PHONY: all clean

clean:
	rm -rf $(BUILD_DIR) $(EXECUTABLE) $(TEST_DB)