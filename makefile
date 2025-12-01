CXX = g++
CXXFLAGS = -Wall -Wextra -g -std=c++17
LDFLAGS = 
TESTS_EXECUTABLE = tests
EXPERIMENTS_EXECUTABLE = experiments

BUILD_DIR = build
SRC_DIR = src
TEST_DIR = test_files
EXPERIMENTS_DIR = experiments_files
TEST_DB = testdb
INCLUDE_DIR = include

LIB_SOURCES = $(wildcard $(SRC_DIR)/*.cpp)

TEST_MAIN = $(TEST_DIR)/mrkdb_tests.cpp

EXPERIMENT_MAIN = $(EXPERIMENTS_DIR)/experiments.cpp

LIB_OBJECTS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(LIB_SOURCES))

TEST_OBJECT = $(BUILD_DIR)/mrkdb_tests.o
EXPERIMENT_OBJECT = $(BUILD_DIR)/experiments.o

TEST_OBJECTS = $(LIB_OBJECTS) $(TEST_OBJECT)

EXPERIMENT_OBJECTS = $(LIB_OBJECTS) $(EXPERIMENT_OBJECT)

OBJECTS = $(LIB_OBJECTS) $(TEST_OBJECT) $(EXPERIMENT_OBJECT)

all: $(BUILD_DIR) $(TESTS_EXECUTABLE) $(EXPERIMENTS_EXECUTABLE)

$(BUILD_DIR):
	mkdir -p $@

$(TESTS_EXECUTABLE): $(TEST_OBJECTS)
	$(CXX) $(TEST_OBJECTS) -o $@ $(LDFLAGS)

$(EXPERIMENTS_EXECUTABLE): $(EXPERIMENT_OBJECTS)
	$(CXX) $(EXPERIMENT_OBJECTS) -o $@ $(LDFLAGS)
	
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

$(BUILD_DIR)/mrkdb_tests.o: $(TEST_DIR)/mrkdb_tests.cpp
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

$(BUILD_DIR)/experiments.o: $(EXPERIMENTS_DIR)/experiments.cpp
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

.PHONY: all clean

clean:
	rm -rf $(BUILD_DIR) $(EXECUTABLE) $(TEST_DB) tests experiments