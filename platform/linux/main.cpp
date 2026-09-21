#include "capture/pipewire_capture.hpp"
#include "capture/screencopy_capture.hpp"
#include "inference/onnx_detector.hpp"
#include "inference/image_preprocessor.hpp"
#include "overlay/wayland_overlay.hpp"
#include "network/dns_server.hpp"
#include "watchdog/anti_tamper.hpp"
#include "ipc/ipc_server.hpp"
#include "engine_shared/temporal_smoothing.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include <csignal>
#include <atomic>
#include <sstream>

using namespace degoonification;

static std::atomic<bool> g_running{true};

static void sigint_handler(int) {
    g_running.store(false);
}

static uint64_t get_current_time_ms() {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

static std::string resolve_model_path(const std::string& override_path) {
    if (!override_path.empty() && access(override_path.c_str(), R_OK) == 0) {
        return override_path;
    }
    const char* home = getenv("HOME");
    std::vector<std::string> candidates;
    if (home) {
        candidates.push_back(std::string(home) + "/.local/share/degoonification/models/yolov8n-nsfw.onnx");
        candidates.push_back(std::string(home) + "/degoonification/core/models/yolov8n-nsfw.onnx");
    }
    candidates.push_back("/usr/local/share/degoonification/models/yolov8n-nsfw.onnx");
    candidates.push_back("core/models/yolov8n-nsfw.onnx");

    for (const auto& path : candidates) {
        if (access(path.c_str(), R_OK) == 0) return path;
    }
    return candidates.empty() ? "core/models/yolov8n-nsfw.onnx" : candidates.front();
}

static std::string resolve_blocklist_path() {
    const char* home = getenv("HOME");
    std::vector<std::string> candidates;
    if (home) {
        candidates.push_back(std::string(home) + "/.local/share/degoonification/blocklists/default_domains.txt");
        candidates.push_back(std::string(home) + "/degoonification/core/blocklists/default_domains.txt");
    }
    candidates.push_back("/usr/local/share/degoonification/blocklists/default_domains.txt");
    candidates.push_back("core/blocklists/default_domains.txt");

    for (const auto& path : candidates) {
        if (access(path.c_str(), R_OK) == 0) return path;
    }
    return candidates.empty() ? "core/blocklists/default_domains.txt" : candidates.front();
}

int main(int argc, char** argv) {
    std::signal(SIGINT, sigint_handler);
    std::signal(SIGTERM, sigint_handler);
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    auto start_time_tp = std::chrono::steady_clock::now();
    auto streak_start_tp = std::chrono::system_clock::now();

    std::cout << "===========================================\n";
    std::cout << "🛡️  Degoonification Linux Daemon (Hyprland/Wayland)\n";
    std::cout << "===========================================\n";

    // 1. Anti-Tamper Security Hardening (Phase 5)
    std::cout << "[Security] Activating process hardening (PR_SET_DUMPABLE=0, RLIMIT_CORE=0)...\n";
    if (watchdog::AntiTamper::harden_current_process()) {
        std::cout << "[Security] Process memory shielded against ptrace and unauthorized dumps.\n";
    }

    // 2. Initialize IPC Server for CLI & TUI
    std::cout << "[IPC] Initializing UNIX Domain Socket IPC Server...\n";
    ipc::IpcServer ipc_server;
    if (ipc_server.init()) {
        std::cout << "[IPC] Listening on: " << ipc_server.socket_path() << "\n";
    } else {
        std::cerr << "[Warning] Failed to initialize IPC server socket.\n";
    }

    // 3. Initialize Sub-millisecond DNS Sinkhole (Phase 4)
    std::string blocklist_path = resolve_blocklist_path();
    std::cout << "[Init] Starting Local DNS Sinkhole on 127.0.0.1:5353 (Blocklist: " << blocklist_path << ")...\n";
    network::DnsServer dns_server(network::DnsServerConfig{
        .listen_ip = "127.0.0.1",
        .listen_port = 5353,
        .upstream_dns_ip = "1.1.1.1",
        .upstream_dns_port = 53,
        .blocklist_path = blocklist_path
    });

    if (dns_server.init() && dns_server.start()) {
        std::cout << "[Init] DNS Sinkhole active on port 5353 (0.0.0.0 adult domain drop).\n";
    } else {
        std::cerr << "[Warning] Failed to bind DNS Sinkhole socket.\n";
    }

    // 4. Initialize Wayland Overlay (Phase 3: Click-through layer shell)
    std::cout << "[Init] Initializing Wayland click-through overlay...\n";
    linux_backend::WaylandOverlay overlay;
    if (!overlay.init()) {
        std::cerr << "[Warning] Wayland overlay init failed (ensure Wayland compositor is running). Continuing in headless mode.\n";
    } else {
        std::cout << "[Init] Overlay initialized successfully (" << overlay.screen_width() << "x" << overlay.screen_height() << ")\n";
    }

    // 5. Initialize Core Shared Anti-Flicker Tracker
    float padding_ratio = 0.15f;
    std::cout << "[Init] Initializing Temporal Smoothing Hysteresis Tracker (300ms window, +15% padding)...\n";
    core::TemporalSmoothingTracker tracker(core::TrackerConfig{
        .persistence_window_ms = 300,
        .padding_ratio = padding_ratio,
        .merge_iou_threshold = 0.25f
    });

    // 6. Initialize Preprocessor
    linux_backend::ImagePreprocessor preprocessor(640, 640);
    std::vector<float> planar_rgb;

    // 7. Initialize AI Inference Engine (Phase 2)
    std::string model_path = resolve_model_path((argc > 1) ? argv[1] : "");
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
        std::cout << "[Info] Running in synthetic test / passthrough mode.\n";
    } else {
        std::cout << "[Init] AI Detector loaded successfully with hardware acceleration.\n";
    }

    // 8. Start Screen Capture & AI Detection Thread
    std::mutex boxes_mutex;
    std::vector<core::BoundingBox> latest_raw_boxes;
    std::atomic<uint64_t> total_frames_analyzed{0};
    std::atomic<uint32_t> current_fps{12};
    std::atomic<bool> worker_running{true};
    bool visual_blur_paused = false;

    std::cout << "[Init] Starting Wayland screen capture & AI detection thread...\n";
    std::thread ai_thread([&]() {
        linux_backend::ScreencopyCapture screencap;
        linux_backend::ImagePreprocessor preprocessor(640, 640);
        linux_backend::VideoFrame frame;
        std::vector<float> planar_rgb;

        uint64_t frame_count = 0;
        uint64_t total_count = 0;
        auto fps_timer = std::chrono::steady_clock::now();

        while (worker_running.load() && g_running.load()) {
            auto loop_start = std::chrono::steady_clock::now();

            if (!visual_blur_paused && model_loaded) {
                if (screencap.capture_frame(frame)) {
                    preprocessor.preprocess(frame, planar_rgb);
                    auto detected_boxes = detector.detect(planar_rgb.data());

                    {
                        std::lock_guard<std::mutex> lock(boxes_mutex);
                        latest_raw_boxes = std::move(detected_boxes);
                    }

                    frame_count++;
                    total_count++;
                    total_frames_analyzed.store(total_count, std::memory_order_relaxed);
                }
            } else {
                std::lock_guard<std::mutex> lock(boxes_mutex);
                latest_raw_boxes.clear();
            }

            auto now_tp = std::chrono::steady_clock::now();
            auto elapsed_sec = std::chrono::duration<double>(now_tp - fps_timer).count();
            if (elapsed_sec >= 1.0) {
                current_fps.store(static_cast<uint32_t>(frame_count / elapsed_sec), std::memory_order_relaxed);
                frame_count = 0;
                fps_timer = now_tp;
            }

            auto process_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - loop_start).count();
            int sleep_ms = 85 - static_cast<int>(process_time); // ~12 FPS
            if (sleep_ms > 10) {
                std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
            }
        }
    });

    std::cout << "[Ready] System active across all subsystems. Press Ctrl+C to exit.\n";

    uint32_t active_box_count = 0;
    std::string last_event = "Defense active and monitoring screen";

    // Main event loop
    while (g_running.load()) {
        uint64_t now_ms = get_current_time_ms();

        // 1. Process IPC commands from CLI / TUI
        ipc_server.process_pending([&](const std::string& cmd, const std::string& args) -> std::string {
            if (cmd == "STATUS") {
                auto now_tp = std::chrono::steady_clock::now();
                uint64_t uptime = std::chrono::duration_cast<std::chrono::seconds>(now_tp - start_time_tp).count();
                auto streak_diff = std::chrono::system_clock::now() - streak_start_tp;
                int streak_days = std::chrono::duration_cast<std::chrono::hours>(streak_diff).count() / 24;
                int streak_hours = (std::chrono::duration_cast<std::chrono::hours>(streak_diff).count()) % 24;
                int streak_minutes = (std::chrono::duration_cast<std::chrono::minutes>(streak_diff).count()) % 60;

                auto stats = dns_server.stats();

                std::ostringstream ss;
                ss << "paused=" << (visual_blur_paused ? "1" : "0") << "\n";
                ss << "active_boxes=" << active_box_count << "\n";
                ss << "fps=" << current_fps.load() << "\n";
                ss << "padding_ratio=" << padding_ratio << "\n";
                ss << "uptime=" << uptime << "\n";
                ss << "total_frames=" << total_frames_analyzed.load() << "\n";
                ss << "dns_total=" << stats.total_queries << "\n";
                ss << "dns_blocked=" << stats.blocked_queries << "\n";
                ss << "dns_forwarded=" << stats.forwarded_queries << "\n";
                ss << "streak_days=" << streak_days << "\n";
                ss << "streak_hours=" << streak_hours << "\n";
                ss << "streak_minutes=" << streak_minutes << "\n";
                ss << "model=" << (model_loaded ? "YOLOv8n-NSFW" : "Passthrough") << "\n";
                ss << "milestone=Dopamine Reset\n";
                ss << "progress=0.45\n";
                ss << "last_event=" << last_event << "\n";
                return ss.str();
            } else if (cmd == "PAUSE") {
                visual_blur_paused = true;
                overlay.render_boxes({});
                last_event = "Visual blur temporarily paused via CLI";
                return "OK\n";
            } else if (cmd == "RESUME") {
                visual_blur_paused = false;
                last_event = "Visual blur resumed via CLI";
                return "OK\n";
            } else if (cmd == "SET_PADDING") {
                try {
                    padding_ratio = std::stof(args);
                    tracker.set_config(core::TrackerConfig{
                        .persistence_window_ms = 300,
                        .padding_ratio = padding_ratio,
                        .merge_iou_threshold = 0.25f
                    });
                    last_event = "Padding ratio adjusted to +" + std::to_string(static_cast<int>(padding_ratio * 100)) + "%";
                } catch (...) {}
                return "OK\n";
            } else if (cmd == "RELAPSE") {
                streak_start_tp = std::chrono::system_clock::now();
                last_event = "Relapse logged: " + args;
                return "OK\n";
            } else if (cmd == "STOP") {
                g_running.store(false);
                last_event = "Daemon stop requested via CLI";
                return "OK\n";
            }
            return "UNKNOWN_COMMAND\n";
        });

        // 2. Process Wayland events non-blockingly
        overlay.dispatch_events();

        // 3. Update temporal smoothing tracker and render boxes onto screen
        std::vector<core::BoundingBox> current_detections;
        {
            std::lock_guard<std::mutex> lock(boxes_mutex);
            current_detections = latest_raw_boxes;
        }

        if (!visual_blur_paused) {
            auto active_boxes = tracker.process_frame(current_detections, now_ms);
            active_box_count = active_boxes.size();
            overlay.render_boxes(active_boxes);
            if (active_box_count > 0) {
                last_event = "BLUR SHIELD ACTIVE: " + std::to_string(active_box_count) + " explicit region(s) blocked";
            }
        } else {
            overlay.render_boxes({});
            active_box_count = 0;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS dispatch
    }

    std::cout << "\n[Shutdown] Cleaning up resources...\n";
    worker_running.store(false);
    if (ai_thread.joinable()) {
        ai_thread.join();
    }
    dns_server.stop();
    ipc_server.stop();
    overlay.render_boxes({}); // Clear screen

    auto stats = dns_server.stats();
    std::cout << "[Stats] DNS Queries processed: " << stats.total_queries
              << " (Blocked: " << stats.blocked_queries
              << ", Forwarded: " << stats.forwarded_queries << ")\n";

    std::cout << "[Shutdown] Degoonification daemon stopped gracefully.\n";
    return 0;
}
