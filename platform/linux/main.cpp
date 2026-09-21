#include "capture/pipewire_capture.hpp"
#include "inference/onnx_detector.hpp"
#include "inference/image_preprocessor.hpp"
#include "overlay/wayland_overlay.hpp"
#include "engine_shared/temporal_smoothing.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <csignal>
#include <atomic>

using namespace degoonification;

static std::atomic<bool> g_running{true};

static void sigint_handler(int) {
    g_running.store(false);
}

static uint64_t get_current_time_ms() {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

int main(int argc, char** argv) {
    std::signal(SIGINT, sigint_handler);
    std::signal(SIGTERM, sigint_handler);

    std::cout << "===========================================\n";
    std::cout << "🛡️  Degoonification Linux Daemon (Hyprland/Wayland)\n";
    std::cout << "===========================================\n";

    // 1. Initialize Wayland Overlay (Click-through layer shell)
    std::cout << "[Init] Initializing Wayland click-through overlay...\n";
    linux_backend::WaylandOverlay overlay;
    if (!overlay.init()) {
        std::cerr << "[Warning] Wayland overlay init failed (ensure Wayland compositor is running). Continuing in headless mode.\n";
    } else {
        std::cout << "[Init] Overlay initialized successfully (" << overlay.screen_width() << "x" << overlay.screen_height() << ")\n";
    }

    // 2. Initialize Core Shared Anti-Flicker Tracker
    std::cout << "[Init] Initializing Temporal Smoothing Hysteresis Tracker (300ms window, +15% padding)...\n";
    core::TemporalSmoothingTracker tracker(core::TrackerConfig{
        .persistence_window_ms = 300,
        .padding_ratio = 0.15f,
        .merge_iou_threshold = 0.25f
    });

    // 3. Initialize Preprocessor
    linux_backend::ImagePreprocessor preprocessor(640, 640);
    std::vector<float> planar_rgb;

    // 4. Initialize AI Inference Engine
    std::string model_path = (argc > 1) ? argv[1] : "core/models/yolov8n-nsfw.onnx";
    std::cout << "[Init] Initializing ONNX Runtime AI Detector with model: " << model_path << "...\n";
    linux_backend::OnnxDetector detector(linux_backend::DetectorConfig{
        .model_path = model_path,
        .confidence_threshold = 0.35f,
        .nms_iou_threshold = 0.45f,
        .input_width = 640,
        .input_height = 640,
        .num_threads = 4,
        .use_gpu = true
    });

    bool model_loaded = detector.init();
    if (!model_loaded) {
        std::cout << "[Info] AI model file not yet downloaded. Running in synthetic test / passthrough mode.\n";
    } else {
        std::cout << "[Init] AI Detector loaded successfully with hardware acceleration.\n";
    }

    // 5. Initialize PipeWire Capture
    std::cout << "[Init] Initializing PipeWire DMA-BUF screen capture...\n";
    linux_backend::PipeWireCapture capture(linux_backend::PipeWireConfig{
        .target_node_id = 0,
        .target_fps = 15,
        .request_dmabuf = true
    });

    if (!capture.init()) {
        std::cerr << "[Warning] PipeWire capture init failed. Running event loop in idle mode.\n";
    }

    std::cout << "[Ready] System active. Press Ctrl+C to exit.\n";

    // Main event loop
    uint64_t last_tick = get_current_time_ms();
    while (g_running.load()) {
        uint64_t now_ms = get_current_time_ms();

        // Process Wayland events
        overlay.dispatch_events();

        // Periodic tracker update / box expiration
        if (now_ms - last_tick >= 100) {
            auto active_boxes = tracker.process_frame({}, now_ms);
            overlay.render_boxes(active_boxes);
            last_tick = now_ms;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS event dispatch
    }

    std::cout << "\n[Shutdown] Cleaning up resources...\n";
    capture.stop();
    overlay.render_boxes({}); // Clear screen

    std::cout << "[Shutdown] Degoonification daemon stopped gracefully.\n";
    return 0;
}
