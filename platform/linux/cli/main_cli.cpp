#include "ipc_client.hpp"
#include "tui_dashboard.hpp"
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <unistd.h>

using namespace degoonification;

static void print_banner() {
    std::cout << "\033[1;36m🛡️  DEGOONIFICATION CLI \033[0m\033[90m// Real-Time System Defense & Self-Control\033[0m\n\n";
}

static void print_help() {
    print_banner();
    std::cout << "\033[1;37mUSAGE:\033[0m\n";
    std::cout << "  degoon [COMMAND] [OPTIONS]\n\n";
    std::cout << "\033[1;37mCOMMANDS:\033[0m\n";
    std::cout << "  \033[1;32mtui\033[0m, \033[1;32mdashboard\033[0m         Launch interactive fullscreen terminal dashboard (live metrics & hotkeys)\n";
    std::cout << "  \033[1;32mstatus\033[0m                 Print current defense status, telemetry, and streak overview\n";
    std::cout << "  \033[1;32mstart\033[0m                  Start the background daemon\n";
    std::cout << "  \033[1;32mstop\033[0m                   Stop the daemon cleanly\n";
    std::cout << "  \033[1;32mrestart\033[0m                Restart the daemon\n";
    std::cout << "  \033[1;32mpause\033[0m                  Temporarily pause visual screen blur\n";
    std::cout << "  \033[1;32mresume\033[0m                 Resume visual screen blur\n";
    std::cout << "  \033[1;32mstreak\033[0m                 Display days clean, milestones, and dopamine recovery progress\n";
    std::cout << "  \033[1;32mrelapse\033[0m [reason]       Log a relapse, record trigger notes, and reset streak\n";
    std::cout << "  \033[1;32mpadding\033[0m <ratio>        Set dynamic bounding box expansion ratio (e.g. 0.15 for +15%)\n";
    std::cout << "  \033[1;32mhelp\033[0m, \033[1;32m--help\033[0m           Show this help message\n\n";
}

static void cmd_status(cli::IpcClient& client) {
    print_banner();
    auto status = client.get_status();
    if (!status) {
        std::cout << "\033[1;31m● Daemon Status: OFFLINE\033[0m\n";
        std::cout << "  The daemon is not currently running.\n";
        std::cout << "  Run '\033[1;32mdegoon start\033[0m' or '\033[1;32m./run.sh\033[0m' to launch.\n";
        return;
    }

    std::cout << "\033[1;32m● Daemon Status: ACTIVE & SHIELDED\033[0m\n";
    std::cout << "  Uptime:            \033[1;37m" << status->uptime_seconds / 3600 << "h "
              << (status->uptime_seconds % 3600) / 60 << "m "
              << (status->uptime_seconds % 60) << "s\033[0m\n";
    std::cout << "  Visual Blur:       " << (status->visual_blur_paused ? "\033[1;33mPaused\033[0m" : "\033[1;32mActive (Overlay)\033[0m")
              << " (" << status->active_boxes << " active boxes)\n";
    std::cout << "  AI Model:          \033[1;35m" << status->active_model << "\033[0m (Inference active)\n";
    std::cout << "  Screen Capture:    \033[1;32m" << status->fps << " FPS\033[0m (PipeWire DMA-BUF zero-copy)\n";
    std::cout << "  Dynamic Expansion: \033[1;37m+" << static_cast<int>(status->padding_ratio * 100) << "%\033[0m\n";
    std::cout << "  DNS Sinkhole:      \033[1;32m127.0.0.1:5353\033[0m (Blocked: " << status->dns_blocked << ", Total: " << status->dns_total << ")\n";
    std::cout << "  Clean Streak:      \033[1;33m" << status->streak_days << " Days, " << status->streak_hours << "h " << status->streak_minutes << "m\033[0m\n";
    std::cout << "  Current Milestone: \033[1;36m" << status->current_milestone << "\033[0m ("
              << static_cast<int>(status->milestone_progress * 100) << "% to next)\n";
}

static void cmd_streak(cli::IpcClient& client) {
    print_banner();
    auto status = client.get_status();
    int days = status ? status->streak_days : 0;
    int hours = status ? status->streak_hours : 0;
    int minutes = status ? status->streak_minutes : 0;
    float progress = status ? status->milestone_progress : 0.0f;
    std::string milestone = status ? status->current_milestone : "First Step";

    std::cout << "\033[1;33m========================================\n";
    std::cout << "           🔥 STREAK TRACKER            \n";
    std::cout << "========================================\033[0m\n\n";

    std::cout << "       \033[1;37m" << days << " DAYS CLEAN\033[0m\n";
    std::cout << "       \033[90m(" << hours << " hours, " << minutes << " minutes)\033[0m\n\n";

    int bar_width = 30;
    int filled = static_cast<int>(progress * bar_width);
    std::string bar;
    for (int i = 0; i < filled; ++i) bar += "█";
    for (int i = filled; i < bar_width; ++i) bar += "░";

    std::cout << "Milestone: \033[1;36m" << milestone << "\033[0m\n";
    std::cout << "Progress:  [\033[1;32m" << bar << "\033[0m] " << static_cast<int>(progress * 100) << "%\n\n";

    std::cout << "\033[90mNeuroplasticity stages:\n";
    std::cout << " - Day 1-3:  Dopamine receptor sensitization starts\n";
    std::cout << " - Day 7:    Craving frequency reduces by ~40%\n";
    std::cout << " - Day 14:   Prefrontal cortex executive control restored\n";
    std::cout << " - Day 30:   Habitual craving loop structurally overhauled\n";
    std::cout << " - Day 90:   Full baseline neuroplastic reboot\033[0m\n";
}

int main(int argc, char** argv) {
    cli::IpcClient client;

    if (argc < 2) {
        // Default action: if running, show TUI dashboard; if not running, show status/help
        if (client.is_daemon_running()) {
            cli::TuiDashboard tui(client);
            tui.run();
        } else {
            cmd_status(client);
        }
        return 0;
    }

    std::string cmd = argv[1];

    if (cmd == "tui" || cmd == "dashboard" || cmd == "monitor") {
        cli::TuiDashboard tui(client);
        tui.run();
    } else if (cmd == "status") {
        cmd_status(client);
    } else if (cmd == "streak") {
        cmd_streak(client);
    } else if (cmd == "pause") {
        if (client.pause()) std::cout << "✓ Visual screen blur paused.\n";
        else std::cerr << "✗ Failed to pause. Is daemon running?\n";
    } else if (cmd == "resume") {
        if (client.resume()) std::cout << "✓ Visual screen blur resumed.\n";
        else std::cerr << "✗ Failed to resume. Is daemon running?\n";
    } else if (cmd == "padding") {
        if (argc < 3) {
            std::cerr << "Usage: degoon padding <ratio> (e.g. 0.15 for +15%)\n";
            return 1;
        }
        float p = std::stof(argv[2]);
        if (client.set_padding(p)) std::cout << "✓ Padding ratio updated to +" << static_cast<int>(p * 100) << "%.\n";
        else std::cerr << "✗ Failed to set padding. Is daemon running?\n";
    } else if (cmd == "relapse") {
        std::string reason = (argc >= 3) ? argv[2] : "Manual CLI reset";
        if (client.log_relapse(reason)) {
            std::cout << "✓ Relapse logged. Streak reset. Stay strong and focus on the next 24 hours.\n";
        } else {
            std::cerr << "✗ Failed to log relapse.\n";
        }
    } else if (cmd == "stop") {
        if (client.stop_daemon()) {
            std::cout << "✓ Degoonification daemon stopped gracefully.\n";
        } else {
            std::cerr << "✗ Failed to stop daemon or already stopped.\n";
        }
    } else if (cmd == "start") {
        if (client.is_daemon_running()) {
            std::cout << "Daemon is already running.\n";
        } else {
            std::cout << "Starting daemon in background...\n";
            int ret = system("nohup ./platform/linux/bin/degoonification-daemon >/dev/null 2>&1 &");
            (void)ret;
            usleep(300000); // 300ms
            if (client.is_daemon_running()) {
                std::cout << "✓ Degoonification daemon started successfully!\n";
            } else {
                std::cout << "✓ Launch command initiated. Check status with 'degoon status'.\n";
            }
        }
    } else if (cmd == "restart") {
        client.stop_daemon();
        usleep(400000);
        system("nohup ./platform/linux/bin/degoonification-daemon >/dev/null 2>&1 &");
        usleep(300000);
        std::cout << "✓ Daemon restarted.\n";
    } else {
        print_help();
    }

    return 0;
}
