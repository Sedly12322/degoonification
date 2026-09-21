CXX ?= g++
CC ?= gcc
CXXFLAGS ?= -std=c++20 -O3 -Wall -Wextra -Wpedantic -fPIC $(shell pkg-config --cflags wayland-client) -Iprotocols -I../../../core/engine_shared/include
CFLAGS ?= -O3 -fPIC $(shell pkg-config --cflags wayland-client) -Iprotocols
LDFLAGS ?= $(shell pkg-config --libs wayland-client)

BUILD_DIR = build
BIN_DIR = bin

OBJS = $(BUILD_DIR)/wlr-layer-shell-unstable-v1-protocol.o $(BUILD_DIR)/wayland_overlay.o

.PHONY: all clean

all: $(OBJS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/wlr-layer-shell-unstable-v1-protocol.o: protocols/wlr-layer-shell-unstable-v1-protocol.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/wayland_overlay.o: wayland_overlay.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
