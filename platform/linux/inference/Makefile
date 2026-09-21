CXX ?= g++
CXXFLAGS ?= -std=c++20 -O3 -Wall -Wextra -Wpedantic -fPIC -I/usr/include/onnxruntime -I../../../core/engine_shared/include
LDFLAGS ?= -lonnxruntime

BUILD_DIR = build
BIN_DIR = bin

OBJS = $(BUILD_DIR)/onnx_detector.o

.PHONY: all clean

all: $(OBJS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
