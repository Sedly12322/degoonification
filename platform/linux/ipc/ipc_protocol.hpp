#pragma once

#include <string>
#include <unistd.h>
#include <cstdint>

namespace degoonification::ipc {

inline std::string get_socket_path() {
    const char* xdg_runtime = getenv("XDG_RUNTIME_DIR");
    if (xdg_runtime && *xdg_runtime) {
        return std::string(xdg_runtime) + "/degoon.sock";
    }
    return "/tmp/degoon_" + std::to_string(getuid()) + ".sock";
}

struct DaemonStatus {
    bool running{false};
    bool visual_blur_paused{false};
    uint32_t active_boxes{0};
    uint32_t fps{15};
    float padding_ratio{0.15f};
    uint64_t uptime_seconds{0};
    uint64_t total_frames{0};
    uint64_t dns_total{0};
    uint64_t dns_blocked{0};
    uint64_t dns_forwarded{0};
    int streak_days{0};
    int streak_hours{0};
    int streak_minutes{0};
    std::string active_model{"yolov8n-nsfw"};
    std::string current_milestone{"First Step"};
    float milestone_progress{0.0f};
    std::string last_event{"Daemon initialized"};
};

} // namespace degoonification::ipc
