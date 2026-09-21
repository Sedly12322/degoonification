#include "ipc_server.hpp"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <iostream>
#include <cstring>
#include <sstream>

namespace degoonification::ipc {

IpcServer::IpcServer(std::string socket_path)
    : socket_path_(socket_path.empty() ? get_socket_path() : std::move(socket_path)) {}

IpcServer::~IpcServer() {
    stop();
}

bool IpcServer::init() {
    // Unlink any existing stale socket
    unlink(socket_path_.c_str());

    server_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        std::cerr << "[IpcServer] socket() failed: " << std::strerror(errno) << "\n";
        return false;
    }

    // Set non-blocking
    int flags = fcntl(server_fd_, F_GETFL, 0);
    fcntl(server_fd_, F_SETFL, flags | O_NONBLOCK);

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, socket_path_.c_str(), sizeof(addr.sun_path) - 1);

    if (bind(server_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "[IpcServer] bind(" << socket_path_ << ") failed: " << std::strerror(errno) << "\n";
        close(server_fd_);
        server_fd_ = -1;
        return false;
    }

    if (listen(server_fd_, 8) < 0) {
        std::cerr << "[IpcServer] listen() failed: " << std::strerror(errno) << "\n";
        close(server_fd_);
        server_fd_ = -1;
        return false;
    }

    return true;
}

void IpcServer::process_pending(CommandHandler handler) {
    if (server_fd_ < 0) return;

    struct pollfd pfd{};
    pfd.fd = server_fd_;
    pfd.events = POLLIN;

    if (poll(&pfd, 1, 0) <= 0 || !(pfd.revents & POLLIN)) {
        return;
    }

    int client_fd = accept(server_fd_, nullptr, nullptr);
    if (client_fd < 0) return;

    char buffer[2048];
    ssize_t bytes = read(client_fd, buffer, sizeof(buffer) - 1);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        std::string req(buffer);
        // Trim trailing newline or whitespace
        while (!req.empty() && (req.back() == '\n' || req.back() == '\r' || req.back() == ' ')) {
            req.pop_back();
        }

        std::string cmd;
        std::string args;
        auto space = req.find(' ');
        if (space != std::string::npos) {
            cmd = req.substr(0, space);
            args = req.substr(space + 1);
        } else {
            cmd = req;
        }

        std::string resp = handler(cmd, args);
        if (resp.empty()) resp = "OK\n";
        if (resp.back() != '\n') resp.push_back('\n');

        write(client_fd, resp.data(), resp.size());
    }

    close(client_fd);
}

void IpcServer::stop() {
    if (server_fd_ >= 0) {
        close(server_fd_);
        server_fd_ = -1;
        unlink(socket_path_.c_str());
    }
}

} // namespace degoonification::ipc
