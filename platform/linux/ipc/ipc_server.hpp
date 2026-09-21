#pragma once

#include "ipc_protocol.hpp"
#include <functional>
#include <string>
#include <memory>
#include <atomic>

namespace degoonification::ipc {

using CommandHandler = std::function<std::string(const std::string& cmd, const std::string& args)>;

class IpcServer {
public:
    explicit IpcServer(std::string socket_path = "");
    ~IpcServer();

    IpcServer(const IpcServer&) = delete;
    IpcServer& operator=(const IpcServer&) = delete;

    /**
     * Creates and binds the UNIX domain socket.
     */
    bool init();

    /**
     * Polls and processes incoming client requests without blocking.
     */
    void process_pending(CommandHandler handler);

    /**
     * Unlinks socket and closes fd.
     */
    void stop();

    [[nodiscard]] bool is_listening() const noexcept { return server_fd_ >= 0; }
    [[nodiscard]] const std::string& socket_path() const noexcept { return socket_path_; }

private:
    std::string socket_path_;
    int server_fd_{-1};
};

} // namespace degoonification::ipc
