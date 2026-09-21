#include "anti_tamper.hpp"
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <fcntl.h>
#include <unistd.h>
#include <csignal>
#include <iostream>

namespace degoonification::watchdog {

bool AntiTamper::harden_current_process() noexcept {
    bool ok = true;

    // 1. Prevent non-root ptrace, debugging, and memory dumping via /proc/self/mem
    if (prctl(PR_SET_DUMPABLE, 0, 0, 0, 0) < 0) {
        ok = false;
    }

    // 2. Set RLIMIT_CORE to 0 to prevent coredump file creation
    struct rlimit rl{};
    rl.rlim_cur = 0;
    rl.rlim_max = 0;
    if (setrlimit(RLIMIT_CORE, &rl) < 0) {
        ok = false;
    }

    // 3. Ignore signals commonly used to detach background daemons
    std::signal(SIGHUP, SIG_IGN);
    std::signal(SIGPIPE, SIG_IGN);

    return ok;
}

bool AntiTamper::set_file_immutable(const std::string& filepath, bool immutable) noexcept {
    int fd = open(filepath.c_str(), O_RDONLY);
    if (fd < 0) return false;

    int flags = 0;
    if (ioctl(fd, FS_IOC_GETFLAGS, &flags) < 0) {
        close(fd);
        return false;
    }

    if (immutable) {
        flags |= FS_IMMUTABLE_FL;
    } else {
        flags &= ~FS_IMMUTABLE_FL;
    }

    int res = ioctl(fd, FS_IOC_SETFLAGS, &flags);
    close(fd);
    return res == 0;
}

} // namespace degoonification::watchdog
