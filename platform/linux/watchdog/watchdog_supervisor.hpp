#pragma once

#include <functional>
#include <atomic>

namespace degoonification::watchdog {

using WorkerFunction = std::function<int()>;

/**
 * Dual-process Watchdog Supervisor.
 * Forks a worker child process and monitors its lifecycle.
 * If the worker process is killed or crashes, the supervisor restarts it within 100 ms.
 */
class WatchdogSupervisor {
public:
    explicit WatchdogSupervisor(int max_restarts = 100);

    /**
     * Spawns worker and keeps it running. Exits only on SIGINT/SIGTERM sent to supervisor.
     */
    int run(WorkerFunction worker);

private:
    int max_restarts_;
    static std::atomic<bool> g_stop_requested;

    static void signal_handler(int sig);
};

} // namespace degoonification::watchdog
