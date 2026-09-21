CXX ?= g++
CXXFLAGS ?= -std=c++20 -O3 -Wall -Wextra -Wno-pedantic -fPIC $(shell pkg-config --cflags libpipewire-0.3)
LDFLAGS ?= $(shell pkg-config --libs libpipewire-0.3)

BUILD_DIR = build
BIN_DIR = bin

OBJS = $(BUILD_DIR)/pipewire_capture.o

.PHONY: all clean

all: $(OBJS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
