CXX ?= g++
CXXFLAGS ?= -std=c++20 -O3 -Wall -Wextra -Wpedantic -fPIC -I.
AR ?= ar

BUILD_DIR = build
BIN_DIR = bin

OBJS = $(BUILD_DIR)/anti_tamper.o $(BUILD_DIR)/watchdog_supervisor.o
LIB_A = $(BIN_DIR)/libwatchdog.a
TEST_BIN = $(BIN_DIR)/test_anti_tamper

.PHONY: all clean test

all: $(LIB_A)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(LIB_A): $(OBJS) | $(BIN_DIR)
	$(AR) rcs $@ $(OBJS)

$(TEST_BIN): tests/test_anti_tamper.cpp $(LIB_A) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< $(LIB_A) -o $@

test: $(TEST_BIN)
	@echo "=== Running Watchdog & Anti-Tamper Tests ==="
	@$(TEST_BIN)
	@echo "=== Watchdog Tests Passed! ==="

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
