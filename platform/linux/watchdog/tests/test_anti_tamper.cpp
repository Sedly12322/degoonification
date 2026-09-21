#include "../anti_tamper.hpp"
#include <cassert>
#include <iostream>
#include <sys/prctl.h>

using namespace degoonification::watchdog;

void test_anti_tamper_harden() {
    bool ok = AntiTamper::harden_current_process();
    assert(ok);

    // Verify PR_GET_DUMPABLE is 0
    int dumpable = prctl(PR_GET_DUMPABLE, 0, 0, 0, 0);
    assert(dumpable == 0);

    std::cout << "[PASS] test_anti_tamper_harden (PR_GET_DUMPABLE == 0)\n";
}

int main() {
    test_anti_tamper_harden();
    std::cout << "All Anti-Tamper tests passed!\n";
    return 0;
}
