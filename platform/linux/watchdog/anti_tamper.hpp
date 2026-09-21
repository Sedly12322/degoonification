#pragma once

#include <string>

namespace degoonification::watchdog {

/**
 * Hardened Anti-Tamper safeguard routines.
 */
class AntiTamper {
public:
    /**
     * Hardens current process:
     * - prctl(PR_SET_DUMPABLE, 0): disables ptrace, debugger attachment, and memory dumps
     * - Disables core dump generation
     * - Ignores non-fatal termination signals (SIGHUP, SIGPIPE)
     */
    static bool harden_current_process() noexcept;

    /**
     * Locks a file with the Linux ext4/btrfs immutable flag (equivalent to chattr +i)
     * preventing deletion, renaming, or modification.
     */
    static bool set_file_immutable(const std::string& filepath, bool immutable) noexcept;
};

} // namespace degoonification::watchdog
