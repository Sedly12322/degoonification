#include "tui_dashboard.hpp"
#include <termios.h>
#include <unistd.h>
#include <poll.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <sstream>
#include <iomanip>

namespace degoonification::cli {

static struct termios orig_termios;

TuiDashboard::TuiDashboard(IpcClient& client)
    : client_(client) {}

TuiDashboard::~TuiDashboard() {
    disable_raw_mode();
}

void TuiDashboard::enable_raw_mode() {
    if (raw_mode_enabled_) return;

    if (tcgetattr(STDIN_FILENO, &orig_termios) == 0) {
        struct termios raw = orig_termios;
        raw.c_lflag &= ~(ECHO | ICANON | ISIG); // Disable echo, canonical mode, signals
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
        raw_mode_enabled_ = true;
    }

    // Switch to alternate screen buffer, hide cursor, clear screen
    std::cout << "\033[?1049h\033[?25l\033[H\033[2J" << std::flush;
}

void TuiDashboard::disable_raw_mode() {
    if (raw_mode_enabled_) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
        raw_mode_enabled_ = false;
        // Exit alternate screen buffer, show cursor
        std::cout << "\033[?1049l\033[?25h" << std::flush;
    }
}

std::string TuiDashboard::format_time(uint64_t seconds) {
    uint64_t d = seconds / 86400;
    uint64_t h = (seconds % 86400) / 3600;
    uint64_t m = (seconds % 3600) / 60;
    uint64_t s = seconds % 60;

    std::ostringstream oss;
    if (d > 0) oss << d << "d ";
    if (h > 0 || d > 0) oss << h << "h ";
    oss << m << "m " << s << "s";
    return oss.str();
}

void TuiDashboard::render(const ipc::DaemonStatus& status) {
    std::ostringstream buf;
    // Move cursor to home without flickering
    buf << "\033[H";

    // 1. Header Bar
    buf << "\033[1;36m┌─────────────────────────────────────────────────────────────────────────────┐\033[0m\n";
    buf << "\033[1;36m│\033[0m  \033[1;37m🛡️  DEGOONIFICATION\033[0m \033[90m//\033[0m \033[1;32mREAL-TIME SYSTEM DEFENSE & STREAK MONITOR\033[0m        \033[1;36m│\033[0m\n";
    buf << "\033[1;36m├─────────────────────────────────────────────────────────────────────────────┤\033[0m\n";

    // Uptime & Status Banner
    buf << "\033[1;36m│\033[0m  Status: ";
    if (status.visual_blur_paused) {
        buf << "\033[1;33m[ PAUSED ]\033[0m       ";
    } else {
        buf << "\033[1;32m[ ACTIVE & SHIELDED ]\033[0m";
    }
    buf << "    Uptime: \033[1;37m" << std::setw(14) << std::left << format_time(status.uptime_seconds) << "\033[0m";
    buf << "  Model: \033[1;35m" << std::setw(14) << std::left << status.active_model << "\033[0m\033[1;36m│\033[0m\n";
    buf << "\033[1;36m├──────────────────────────────────────┬──────────────────────────────────────┤\033[0m\n";

    // 2. Dual Panel Layout: Left = Defense Telemetry, Right = Streak / Recovery
    buf << "\033[1;36m│\033[0m \033[1;34m⚡ REAL-TIME DEFENSE TELEMETRY\033[0m        \033[1;36m│\033[0m \033[1;33m🔥 STREAK & NEUROPLASTIC REBOOT\033[0m     \033[1;36m│\033[0m\n";
    buf << "\033[1;36m├──────────────────────────────────────┼──────────────────────────────────────┤\033[0m\n";

    // Line 1
    buf << "\033[1;36m│\033[0m  Visual Blur: "
        << (status.visual_blur_paused ? "\033[1;33mPaused              \033[0m" : "\033[1;32mActive (Overlay)    \033[0m")
        << "  \033[1;36m│\033[0m  Clean: \033[1;37m"
        << std::setw(3) << status.streak_days << " Days "
        << std::setw(2) << status.streak_hours << "h "
        << std::setw(2) << status.streak_minutes << "m\033[0m               \033[1;36m│\033[0m\n";

    // Line 2
    buf << "\033[1;36m│\033[0m  Active Blur Boxes: \033[1;37m" << std::setw(4) << std::left << status.active_boxes << "\033[0m            \033[1;36m│\033[0m  Milestone: \033[1;33m"
        << std::setw(23) << std::left << status.current_milestone << "\033[0m   \033[1;36m│\033[0m\n";

    // Line 3: Progress Bar
    int bar_width = 24;
    int filled = static_cast<int>(status.milestone_progress * bar_width);
    if (filled < 0) filled = 0;
    if (filled > bar_width) filled = bar_width;

    std::string progress_bar;
    for (int i = 0; i < filled; ++i) progress_bar += "█";
    for (int i = filled; i < bar_width; ++i) progress_bar += "░";

    buf << "\033[1;36m│\033[0m  Screen Capture: \033[1;32m" << std::setw(3) << status.fps << " FPS\033[0m (DMA-BUF)    \033[1;36m│\033[0m  ["
        << "\033[1;32m" << progress_bar << "\033[0m] "
        << std::setw(3) << static_cast<int>(status.milestone_progress * 100) << "%    \033[1;36m│\033[0m\n";

    // Line 4
    buf << "\033[1;36m│\033[0m  Box Padding: \033[1;37m+" << static_cast<int>(status.padding_ratio * 100) << "%\033[0m (Hysteresis 300ms) \033[1;36m│\033[0m                                      \033[1;36m│\033[0m\n";

    // Line 5: DNS Stats
    buf << "\033[1;36m│\033[0m  DNS Sinkhole: \033[1;32mPort 5353 (0.0.0.0)\033[0m   \033[1;36m│\033[0m  Dopamine baseline stabilizing...    \033[1;36m│\033[0m\n";
    buf << "\033[1;36m│\033[0m  DNS Blocked: \033[1;31m" << std::setw(6) << std::left << status.dns_blocked << "\033[0m (Total: " << std::setw(6) << status.dns_total << ")  \033[1;36m│\033[0m  Urges reduce, focus improves.       \033[1;36m│\033[0m\n";
    buf << "\033[1;36m│\033[0m  Frames Analyzed: \033[1;37m" << std::setw(15) << std::left << status.total_frames << "\033[0m    \033[1;36m│\033[0m  Air-Gapped: Zero network leak.      \033[1;36m│\033[0m\n";
    buf << "\033[1;36m├──────────────────────────────────────┴──────────────────────────────────────┤\033[0m\n";

    // 3. Live Log / Event feed
    buf << "\033[1;36m│\033[0m \033[1;37m📜 LAST SYSTEM EVENT:\033[0m \033[90m" << std::setw(52) << std::left << status.last_event.substr(0, 52) << "\033[0m\033[1;36m│\033[0m\n";
    buf << "\033[1;36m├─────────────────────────────────────────────────────────────────────────────┤\033[0m\n";

    // 4. Interactive Keybinds
    buf << "\033[1;36m│\033[0m  \033[1;33m[p]\033[0m " << (status.visual_blur_paused ? "Resume Blur  " : "Pause Blur   ")
        << "\033[1;33m[+]\033[0m/\033[1;33m[-]\033[0m Padding   "
        << "\033[1;33m[r]\033[0m Log Relapse   "
        << "\033[1;33m[q]\033[0m Exit TUI (keeps daemon) \033[1;36m│\033[0m\n";
    buf << "\033[1;36m└─────────────────────────────────────────────────────────────────────────────┘\033[0m\n";

    std::cout << buf.str() << std::flush;
}

void TuiDashboard::run() {
    enable_raw_mode();

    struct pollfd pfd{};
    pfd.fd = STDIN_FILENO;
    pfd.events = POLLIN;

    bool running = true;
    while (running) {
        auto status = client_.get_status();
        if (!status) {
            ipc::DaemonStatus fallback;
            fallback.running = false;
            fallback.last_event = "Waiting for daemon connection at " + ipc::get_socket_path();
            render(fallback);
        } else {
            render(*status);
        }

        // Check for user keypress (250 ms timeout for ~4 FPS refresh)
        if (poll(&pfd, 1, 250) > 0 && (pfd.revents & POLLIN)) {
            char c = 0;
            if (read(STDIN_FILENO, &c, 1) > 0) {
                switch (c) {
                    case 'q':
                    case 'Q':
                    case 27: // ESC
                        running = false;
                        break;
                    case 'p':
                    case 'P':
                        if (status) {
                            if (status->visual_blur_paused) client_.resume();
                            else client_.pause();
                        }
                        break;
                    case '+':
                    case '=':
                        if (status) {
                            float new_pad = std::min(0.35f, status->padding_ratio + 0.05f);
                            client_.set_padding(new_pad);
                        }
                        break;
                    case '-':
                    case '_':
                        if (status) {
                            float new_pad = std::max(0.05f, status->padding_ratio - 0.05f);
                            client_.set_padding(new_pad);
                        }
                        break;
                    case 'r':
                    case 'R':
                        client_.log_relapse("TUI reset");
                        break;
                }
            }
        }
    }

    disable_raw_mode();
}

} // namespace degoonification::cli
