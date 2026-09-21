CXX ?= g++
CXXFLAGS ?= -std=c++20 -O3 -Wall -Wextra -Wpedantic -fPIC -I. -I../../../core/engine_shared/include
AR ?= ar

BUILD_DIR = build
BIN_DIR = bin

OBJS = $(BUILD_DIR)/dns_packet.o $(BUILD_DIR)/dns_server.o
LIB_A = $(BIN_DIR)/libdns_sinkhole.a
TEST_BIN = $(BIN_DIR)/test_dns_packet

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

$(TEST_BIN): tests/test_dns_packet.cpp $(LIB_A) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< $(LIB_A) -o $@

test: $(TEST_BIN)
	@echo "=== Running DNS Network Test Suite ==="
	@$(TEST_BIN)
	@echo "=== DNS Network Tests Passed! ==="

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
