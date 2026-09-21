#pragma once

#include "ipc_client.hpp"

namespace degoonification::cli {

class TuiDashboard {
public:
    explicit TuiDashboard(IpcClient& client);
    ~TuiDashboard();

    /**
     * Starts the fullscreen interactive TUI dashboard.
     * Runs until user presses 'q' or Ctrl+C.
     */
    void run();

private:
    IpcClient& client_;
    bool raw_mode_enabled_{false};

    void enable_raw_mode();
    void disable_raw_mode();
    void render(const ipc::DaemonStatus& status);
    static std::string format_time(uint64_t seconds);
};

} // namespace degoonification::cli
