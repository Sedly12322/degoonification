CXX ?= g++
CXXFLAGS ?= -std=c++20 -O3 -Wall -Wextra -Wpedantic -fPIC -Iinclude
AR ?= ar

BUILD_DIR = build
BIN_DIR = bin

SRCS = src/temporal_smoothing.cpp src/bloom_filter.cpp src/radix_tree.cpp src/c_api.cpp
OBJS = $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))

LIB_SO = $(BIN_DIR)/libengine_shared.so
LIB_A = $(BIN_DIR)/libengine_shared.a

TEST_SRCS = $(wildcard tests/*.cpp)
TEST_BINS = $(patsubst tests/%.cpp,$(BIN_DIR)/%,$(TEST_SRCS))

.PHONY: all clean test

all: $(LIB_SO) $(LIB_A)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BUILD_DIR)/%.o: src/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(LIB_SO): $(OBJS) | $(BIN_DIR)
	$(CXX) -shared -o $@ $(OBJS)

$(LIB_A): $(OBJS) | $(BIN_DIR)
	$(AR) rcs $@ $(OBJS)

$(BIN_DIR)/%: tests/%.cpp $(LIB_A) $(LIB_SO) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< -L$(BIN_DIR) -lengine_shared -Wl,-rpath,'$$ORIGIN' -o $@

test: all $(TEST_BINS)
	@echo "=== Running Core Engine Test Suite ==="
	@for t in $(TEST_BINS); do \
		echo "Running $$t..."; \
		$$t || exit 1; \
	done
	@echo "=== All Tests Passed Successfully! ==="

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
