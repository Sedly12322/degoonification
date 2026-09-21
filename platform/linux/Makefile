CXX ?= g++
CC ?= gcc

CXXFLAGS ?= -std=c++20 -O3 -Wall -Wextra -Wno-pedantic -fPIC \
	-I../../core/engine_shared/include \
	-Icapture \
	-Iinference \
	-Ioverlay \
	-Ioverlay/protocols \
	-I/usr/include/onnxruntime \
	$(shell pkg-config --cflags libpipewire-0.3 wayland-client)

CFLAGS ?= -O3 -fPIC $(shell pkg-config --cflags wayland-client) -Ioverlay/protocols

LDFLAGS ?= \
	../../core/engine_shared/bin/libengine_shared.a \
	-lonnxruntime \
	$(shell pkg-config --libs libpipewire-0.3 wayland-client)

BUILD_DIR = build
BIN_DIR = bin

OBJS = \
	$(BUILD_DIR)/pipewire_capture.o \
	$(BUILD_DIR)/onnx_detector.o \
	$(BUILD_DIR)/image_preprocessor.o \
	$(BUILD_DIR)/xdg-shell-protocol.o \
	$(BUILD_DIR)/wlr-layer-shell-unstable-v1-protocol.o \
	$(BUILD_DIR)/wayland_overlay.o \
	$(BUILD_DIR)/main.o

TARGET = $(BIN_DIR)/degoonification-daemon

.PHONY: all clean

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

../../core/engine_shared/bin/libengine_shared.a:
	$(MAKE) -C ../../core/engine_shared

$(BUILD_DIR)/pipewire_capture.o: capture/pipewire_capture.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/onnx_detector.o: inference/onnx_detector.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/image_preprocessor.o: inference/image_preprocessor.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/xdg-shell-protocol.o: overlay/protocols/xdg-shell-protocol.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/wlr-layer-shell-unstable-v1-protocol.o: overlay/protocols/wlr-layer-shell-unstable-v1-protocol.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/wayland_overlay.o: overlay/wayland_overlay.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/main.o: main.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJS) ../../core/engine_shared/bin/libengine_shared.a | $(BIN_DIR)
	$(CXX) $(OBJS) $(LDFLAGS) -o $@

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
