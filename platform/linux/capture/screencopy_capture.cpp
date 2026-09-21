#include "screencopy_capture.hpp"
#include <spawn.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <cstdlib>

extern char** environ;

namespace degoonification::linux_backend {

ScreencopyCapture::ScreencopyCapture() {
    shm_path_ = "/dev/shm/degoon_cap_" + std::to_string(getpid()) + ".ppm";
}

ScreencopyCapture::~ScreencopyCapture() {
    unlink(shm_path_.c_str());
}

bool ScreencopyCapture::is_available() noexcept {
    return access("/usr/bin/grim", X_OK) == 0;
}

bool ScreencopyCapture::capture_frame(VideoFrame& out_frame) {
    pid_t pid;
    char* argv[] = {
        (char*)"grim",
        (char*)"-s", (char*)"1",
        (char*)"-t", (char*)"ppm",
        const_cast<char*>(shm_path_.c_str()),
        nullptr
    };

    if (posix_spawnp(&pid, "grim", nullptr, nullptr, argv, environ) != 0) {
        return false;
    }

    int status = 0;
    if (waitpid(pid, &status, 0) < 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        return false;
    }

    int fd = open(shm_path_.c_str(), O_RDONLY);
    if (fd < 0) return false;

    char header[128];
    ssize_t n = read(fd, header, sizeof(header) - 1);
    if (n < 4 || header[0] != 'P' || header[1] != '6') {
        close(fd);
        return false;
    }
    header[n] = '\0';

    char* ptr = header + 2;
    while (*ptr && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r')) ++ptr;
    int w = std::strtol(ptr, &ptr, 10);
    while (*ptr && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r')) ++ptr;
    int h = std::strtol(ptr, &ptr, 10);
    while (*ptr && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r')) ++ptr;
    int maxval = std::strtol(ptr, &ptr, 10);
    (void)maxval;
    if (*ptr == ' ' || *ptr == '\n' || *ptr == '\r') ++ptr;

    size_t header_len = ptr - header;
    size_t data_size = static_cast<size_t>(w) * h * 3;

    if (buffer_.size() < data_size) {
        buffer_.resize(data_size);
    }

    lseek(fd, header_len, SEEK_SET);
    size_t total_read = 0;
    while (total_read < data_size) {
        ssize_t bytes = read(fd, buffer_.data() + total_read, data_size - total_read);
        if (bytes <= 0) break;
        total_read += bytes;
    }
    close(fd);

    if (total_read != data_size) return false;

    last_width_ = w;
    last_height_ = h;

    out_frame.width = w;
    out_frame.height = h;
    out_frame.stride = w * 3;
    out_frame.format = PixelFormat::RGB;
    out_frame.data = buffer_.data();
    out_frame.data_size = data_size;
    out_frame.dmabuf_fd = -1;

    return true;
}

} // namespace degoonification::linux_backend
