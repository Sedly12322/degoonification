#include "watchdog_supervisor.hpp"
#include <sys/wait.h>
#include <unistd.h>
#include <csignal>
#include <iostream>
#include <chrono>
#include <thread>

namespace degoonification::watchdog {

std::atomic<bool> WatchdogSupervisor::g_stop_requested{false};

void WatchdogSupervisor::signal_handler(int sig) {
    (void)sig;
    g_stop_requested.store(true);
}

WatchdogSupervisor::WatchdogSupervisor(int max_restarts)
    : max_restarts_(max_restarts) {}

int WatchdogSupervisor::run(WorkerFunction worker) {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    int restart_count = 0;

    while (!g_stop_requested.load() && (max_restarts_ <= 0 || restart_count < max_restarts_)) {
        pid_t pid = fork();

        if (pid < 0) {
            std::cerr << "[Watchdog] Fork failed\n";
            return 1;
        }

        if (pid == 0) {
            // Child process: execute the worker function
            return worker();
        }

        // Parent process: monitor child
        std::cout << "[Watchdog] Worker process spawned (PID: " << pid << ")\n";

        int status = 0;
        while (!g_stop_requested.load()) {
            pid_t w = waitpid(pid, &status, WNOHANG);
            if (w == pid) {
                // Child terminated
                if (WIFEXITED(status)) {
                    int exit_code = WEXITSTATUS(status);
                    std::cout << "[Watchdog] Worker exited with code: " << exit_code << "\n";
                    if (exit_code == 0) {
                        return 0; // Normal clean shutdown
                    }
                } else if (WIFSIGNALED(status)) {
                    std::cout << "[Watchdog] Worker terminated by signal: " << WTERMSIG(status) << "!\n";
                }
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        if (g_stop_requested.load()) {
            // Clean shutdown requested: kill child cleanly
            kill(pid, SIGTERM);
            waitpid(pid, nullptr, 0);
            break;
        }

        ++restart_count;
        std::cout << "[Watchdog] Relapse/Termination detected! Instantly relaunching worker (Attempt #"
                  << restart_count << ")...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return 0;
}

} // namespace degoonification::watchdog
