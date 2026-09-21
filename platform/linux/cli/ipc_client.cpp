#include "ipc_client.hpp"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <sstream>

namespace degoonification::cli {

IpcClient::IpcClient(std::string socket_path)
    : socket_path_(socket_path.empty() ? ipc::get_socket_path() : std::move(socket_path)) {}

bool IpcClient::is_daemon_running() {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return false;

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, socket_path_.c_str(), sizeof(addr.sun_path) - 1);

    bool ok = (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0);
    close(fd);
    return ok;
}

std::optional<std::string> IpcClient::send_command(const std::string& command) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return std::nullopt;

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, socket_path_.c_str(), sizeof(addr.sun_path) - 1);

    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(fd);
        return std::nullopt;
    }

    std::string msg = command;
    if (msg.empty() || msg.back() != '\n') msg.push_back('\n');

    if (write(fd, msg.data(), msg.size()) <= 0) {
        close(fd);
        return std::nullopt;
    }

    char buffer[4096];
    std::string response;
    ssize_t bytes = 0;
    while ((bytes = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes] = '\0';
        response.append(buffer);
    }

    close(fd);
    return response;
}

ipc::DaemonStatus IpcClient::parse_status(const std::string& raw) {
    ipc::DaemonStatus s;
    s.running = true;

    std::istringstream stream(raw);
    std::string line;
    while (std::getline(stream, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string k = line.substr(0, eq);
        std::string v = line.substr(eq + 1);

        if (k == "paused") s.visual_blur_paused = (v == "1" || v == "true");
        else if (k == "active_boxes") s.active_boxes = std::stoul(v);
        else if (k == "fps") s.fps = std::stoul(v);
        else if (k == "padding_ratio") s.padding_ratio = std::stof(v);
        else if (k == "uptime") s.uptime_seconds = std::stoull(v);
        else if (k == "total_frames") s.total_frames = std::stoull(v);
        else if (k == "dns_total") s.dns_total = std::stoull(v);
        else if (k == "dns_blocked") s.dns_blocked = std::stoull(v);
        else if (k == "dns_forwarded") s.dns_forwarded = std::stoull(v);
        else if (k == "streak_days") s.streak_days = std::stoi(v);
        else if (k == "streak_hours") s.streak_hours = std::stoi(v);
        else if (k == "streak_minutes") s.streak_minutes = std::stoi(v);
        else if (k == "model") s.active_model = v;
        else if (k == "milestone") s.current_milestone = v;
        else if (k == "progress") s.milestone_progress = std::stof(v);
        else if (k == "last_event") s.last_event = v;
    }
    return s;
}

std::optional<ipc::DaemonStatus> IpcClient::get_status() {
    auto resp = send_command("STATUS");
    if (!resp) return std::nullopt;
    return parse_status(*resp);
}

bool IpcClient::pause() {
    auto r = send_command("PAUSE");
    return r && r->find("OK") != std::string::npos;
}

bool IpcClient::resume() {
    auto r = send_command("RESUME");
    return r && r->find("OK") != std::string::npos;
}

bool IpcClient::set_padding(float ratio) {
    auto r = send_command("SET_PADDING " + std::to_string(ratio));
    return r && r->find("OK") != std::string::npos;
}

bool IpcClient::stop_daemon() {
    auto r = send_command("STOP");
    return r && r->find("OK") != std::string::npos;
}

bool IpcClient::log_relapse(const std::string& reason) {
    auto r = send_command("RELAPSE " + reason);
    return r && r->find("OK") != std::string::npos;
}

} // namespace degoonification::cli
