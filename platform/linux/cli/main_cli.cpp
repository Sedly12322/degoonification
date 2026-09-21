#include "ipc_client.hpp"
#include "tui_dashboard.hpp"
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <unistd.h>
#include <vector>
#include <climits>

using namespace degoonification;

static void print_banner() {
    std::cout << "\033[1;36m🛡️  DEGOONIFICATION CLI \033[0m\033[90m// Real-Time System Defense & Self-Control\033[0m\n\n";
}

static void print_recent_logs(int lines = 20) {
    std::string cmd = "tail -n " + std::to_string(lines) + " /tmp/degoon.log 2>/dev/null";
    system(cmd.c_str());
}

static std::string find_daemon_executable() {
    // 1. Check directory of current executable (/proc/self/exe)
    char self_path[PATH_MAX] = {0};
    ssize_t len = readlink("/proc/self/exe", self_path, sizeof(self_path) - 1);
    if (len > 0) {
        self_path[len] = '\0';
        std::string dir(self_path);
        size_t last_slash = dir.find_last_of('/');
        if (last_slash != std::string::npos) {
            std::string sibling = dir.substr(0, last_slash) + "/degoonification-daemon";
            if (access(sibling.c_str(), X_OK) == 0) {
                return sibling;
            }
        }
    }

    // 2. Check standard install and local paths
    const char* home = getenv("HOME");
    std::vector<std::string> candidates;
    if (home) {
        candidates.push_back(std::string(home) + "/.local/bin/degoonification-daemon");
        candidates.push_back(std::string(home) + "/degoonification/platform/linux/bin/degoonification-daemon");
    }
    candidates.push_back("/usr/local/bin/degoonification-daemon");
    candidates.push_back("./platform/linux/bin/degoonification-daemon");

    for (const auto& path : candidates) {
        if (access(path.c_str(), X_OK) == 0) {
            return path;
        }
    }

    return "degoonification-daemon";
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
    std::cout << "  \033[1;32mlogs\033[0m [lines]           Display daemon log output\n";
    std::cout << "  \033[1;32menable\033[0m                 Enable daemon autostart on login (systemd user service)\n";
    std::cout << "  \033[1;32mdisable\033[0m                Disable daemon autostart on login\n";
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
        bool stopped = false;
        if (client.is_daemon_running()) {
            stopped = client.stop_daemon();
        }
        int sys_ret = system("systemctl --user stop degoonification 2>/dev/null");
        if (stopped || sys_ret == 0) {
            std::cout << "\033[1;32m✓ Degoonification daemon stopped gracefully.\033[0m\n";
        } else {
            std::cout << "Daemon is not currently running.\n";
        }
    } else if (cmd == "start") {
        if (client.is_daemon_running()) {
            std::cout << "\033[1;33m● Daemon is already running.\033[0m\n";
            std::cout << "  Run '\033[1;32mdegoon status\033[0m' or '\033[1;32mdegoon tui\033[0m' to inspect.\n";
        } else {
            std::string daemon_bin = find_daemon_executable();
            std::cout << "Starting degoonification daemon...\n";

            // 1. Try systemd user unit first
            int sys_ret = system("systemctl --user start degoonification 2>/dev/null");
            if (sys_ret != 0) {
                // 2. Try systemd-run --user
                std::string srun_cmd = "systemd-run --user --unit=degoonification " + daemon_bin + " >/dev/null 2>&1";
                sys_ret = system(srun_cmd.c_str());
                if (sys_ret != 0) {
                    // 3. Fallback to nohup
                    std::string launch_cmd = "nohup " + daemon_bin + " >> /tmp/degoon.log 2>&1 &";
                    (void)system(launch_cmd.c_str());
                }
            }

            bool active = false;
            for (int i = 0; i < 15; ++i) {
                usleep(150000); // 150ms
                if (client.is_daemon_running()) {
                    active = true;
                    break;
                }
            }

            if (active) {
                std::cout << "\033[1;32m✓ Degoonification daemon started successfully!\033[0m\n";
                std::cout << "  Socket: " << ipc::get_socket_path() << "\n";
                std::cout << "  Logs:   'degoon logs' or journalctl --user -u degoonification -f\n";
                std::cout << "  Run '\033[1;32mdegoon status\033[0m' or '\033[1;32mdegoon tui\033[0m' to view live metrics.\n";
            } else {
                std::cout << "\033[1;31m✗ Failed to connect to daemon socket after start.\033[0m\n";
                std::cout << "  Recent log output:\n";
                std::cout << "  --------------------------------------------------\n";
                system("journalctl --user -u degoonification -n 15 --no-pager 2>/dev/null || tail -n 15 /tmp/degoon.log 2>/dev/null");
                std::cout << "  --------------------------------------------------\n";
                std::cout << "  Try running manually: " << daemon_bin << "\n";
            }
        }
    } else if (cmd == "restart") {
        if (client.is_daemon_running()) {
            std::cout << "Stopping running daemon...\n";
            client.stop_daemon();
            system("systemctl --user stop degoonification 2>/dev/null");
            for (int i = 0; i < 10; ++i) {
                usleep(150000);
                if (!client.is_daemon_running()) break;
            }
        }
        std::string daemon_bin = find_daemon_executable();
        std::cout << "Starting degoonification daemon...\n";
        int sys_ret = system("systemctl --user restart degoonification 2>/dev/null");
        if (sys_ret != 0) {
            std::string srun_cmd = "systemd-run --user --unit=degoonification " + daemon_bin + " >/dev/null 2>&1";
            sys_ret = system(srun_cmd.c_str());
            if (sys_ret != 0) {
                std::string launch_cmd = "nohup " + daemon_bin + " >> /tmp/degoon.log 2>&1 &";
                (void)system(launch_cmd.c_str());
            }
        }
        bool active = false;
        for (int i = 0; i < 15; ++i) {
            usleep(150000);
            if (client.is_daemon_running()) {
                active = true;
                break;
            }
        }
        if (active) {
            std::cout << "\033[1;32m✓ Daemon restarted successfully.\033[0m\n";
        } else {
            std::cout << "\033[1;31m✗ Daemon failed to restart.\033[0m Check 'degoon logs'.\n";
        }
    } else if (cmd == "enable") {
        int ret = system("systemctl --user enable degoonification 2>/dev/null");
        if (ret == 0) {
            std::cout << "\033[1;32m✓ Autostart enabled:\033[0m Degoonification will start automatically on login.\n";
        } else {
            std::cerr << "✗ Failed to enable autostart. Run 'make install' to set up systemd user service.\n";
        }
    } else if (cmd == "disable") {
        int ret = system("systemctl --user disable degoonification 2>/dev/null");
        if (ret == 0) {
            std::cout << "\033[1;32m✓ Autostart disabled:\033[0m Degoonification will not start automatically on login.\n";
        } else {
            std::cerr << "✗ Failed to disable autostart.\n";
        }
    } else if (cmd == "logs" || cmd == "log") {
        print_banner();
        int lines = (argc >= 3) ? std::atoi(argv[2]) : 30;
        if (lines <= 0) lines = 30;
        std::cout << "\033[1;37mRecent Daemon Logs (last " << lines << " lines):\033[0m\n";
        std::cout << "------------------------------------------------------------\n";
        std::string cmd_str = "journalctl --user -u degoonification -n " + std::to_string(lines) + " --no-pager 2>/dev/null";
        int jret = system(cmd_str.c_str());
        if (jret != 0) {
            print_recent_logs(lines);
        }
        std::cout << "------------------------------------------------------------\n";
    } else {
        print_help();
    }

    return 0;
}
