#include "segv_handler.h"

#include <util/system/backtrace.h>

#ifdef _unix_
#include <cstdlib>
#include <signal.h>
#include <unistd.h>
#endif


namespace NAntiRobot {


#ifdef _unix_
namespace {
    void FancySegvHandler(int sig) {
        Y_UNUSED(sig);

        // Strictly speaking, calling PrintBackTrace isn't safe because it uses
        // functions that are not "async-signal-safe".
        PrintBackTrace();

        if (sig_t old = signal(SIGSEGV, SIG_DFL); old == SIG_ERR) {
            const char msg[] = "FancySegvHandler: signal failed\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            std::abort();
        }
    }
}


void SetFancySegvHandler() {
    signal(SIGSEGV, FancySegvHandler);
}
#else
void SetFancySegvHandler() {}
#endif


}
