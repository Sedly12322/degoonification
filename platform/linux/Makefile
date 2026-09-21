CXX ?= g++
CC ?= gcc

CXXFLAGS ?= -std=c++20 -O3 -Wall -Wextra -Wno-pedantic -fPIC \
	-I../../core/engine_shared/include \
	-Icapture \
	-Iinference \
	-Ioverlay \
	-Ioverlay/protocols \
	-Inetwork \
	-Iwatchdog \
	-Iipc \
	-Icli \
	-I/usr/include/onnxruntime \
	$(shell pkg-config --cflags libpipewire-0.3 wayland-client)

CFLAGS ?= -O3 -fPIC $(shell pkg-config --cflags wayland-client) -Ioverlay/protocols

LDFLAGS_DAEMON ?= \
	../../core/engine_shared/bin/libengine_shared.a \
	-lonnxruntime \
	$(shell pkg-config --libs libpipewire-0.3 wayland-client)

BUILD_DIR = build
BIN_DIR = bin

DAEMON_OBJS = \
	$(BUILD_DIR)/pipewire_capture.o \
	$(BUILD_DIR)/screencopy_capture.o \
	$(BUILD_DIR)/onnx_detector.o \
	$(BUILD_DIR)/image_preprocessor.o \
	$(BUILD_DIR)/xdg-shell-protocol.o \
	$(BUILD_DIR)/wlr-layer-shell-unstable-v1-protocol.o \
	$(BUILD_DIR)/wayland_overlay.o \
	$(BUILD_DIR)/dns_packet.o \
	$(BUILD_DIR)/dns_server.o \
	$(BUILD_DIR)/anti_tamper.o \
	$(BUILD_DIR)/watchdog_supervisor.o \
	$(BUILD_DIR)/ipc_server.o \
	$(BUILD_DIR)/main.o

CLI_OBJS = \
	$(BUILD_DIR)/ipc_client.o \
	$(BUILD_DIR)/tui_dashboard.o \
	$(BUILD_DIR)/main_cli.o

DAEMON_TARGET = $(BIN_DIR)/degoonification-daemon
CLI_TARGET = $(BIN_DIR)/degoon

.PHONY: all clean

all: $(DAEMON_TARGET) $(CLI_TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

../../core/engine_shared/bin/libengine_shared.a:
	$(MAKE) -C ../../core/engine_shared

$(BUILD_DIR)/pipewire_capture.o: capture/pipewire_capture.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/screencopy_capture.o: capture/screencopy_capture.cpp | $(BUILD_DIR)
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

$(BUILD_DIR)/dns_packet.o: network/dns_packet.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/dns_server.o: network/dns_server.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/anti_tamper.o: watchdog/anti_tamper.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/watchdog_supervisor.o: watchdog/watchdog_supervisor.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/ipc_server.o: ipc/ipc_server.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/main.o: main.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/ipc_client.o: cli/ipc_client.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/tui_dashboard.o: cli/tui_dashboard.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/main_cli.o: cli/main_cli.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(DAEMON_TARGET): $(DAEMON_OBJS) ../../core/engine_shared/bin/libengine_shared.a | $(BIN_DIR)
	$(CXX) $(DAEMON_OBJS) $(LDFLAGS_DAEMON) -o $@

$(CLI_TARGET): $(CLI_OBJS) | $(BIN_DIR)
	$(CXX) $(CLI_OBJS) -o $@

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
