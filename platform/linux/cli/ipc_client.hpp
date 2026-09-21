#pragma once

#include "../ipc/ipc_protocol.hpp"
#include <string>
#include <optional>

namespace degoonification::cli {

class IpcClient {
public:
    explicit IpcClient(std::string socket_path = "");

    /**
     * Checks if the daemon UNIX socket is alive and responding.
     */
    [[nodiscard]] bool is_daemon_running();

    /**
     * Sends raw command string and returns response.
     */
    std::optional<std::string> send_command(const std::string& command);

    /**
     * Fetches structured daemon status.
     */
    std::optional<ipc::DaemonStatus> get_status();

    bool pause();
    bool resume();
    bool set_padding(float ratio);
    bool stop_daemon();
    bool log_relapse(const std::string& reason);

private:
    std::string socket_path_;

    static ipc::DaemonStatus parse_status(const std::string& raw_resp);
};

} // namespace degoonification::cli
