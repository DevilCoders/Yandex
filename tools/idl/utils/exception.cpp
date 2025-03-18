#include <yandex/maps/idl/utils/exception.h>

#include <yandex/maps/idl/utils/common.h>

#if defined(__GNUC__) && !defined(_musl_)
#include <execinfo.h>
#include <unistd.h>

#include <csignal>
#include <cstdlib>
#include <iostream>
#endif

namespace yandex {
namespace maps {
namespace idl {
namespace utils {

#if defined(__GNUC__) && !defined(_musl_)
void signal_handler(int signal)
{
    std::cerr << std::endl <<
        "*** *** *** *** *** *** *** *** *** *** *** ***" << std::endl <<
        "Received signal " << signal << std::endl <<
        "Stack trace:" << std::endl;

    constexpr int MAX_STACK_FRAMES = 100;
    void* stackFrames[MAX_STACK_FRAMES];

    const auto size = backtrace(stackFrames, MAX_STACK_FRAMES);
    backtrace_symbols_fd(stackFrames, size, STDERR_FILENO);

    std::exit(EXIT_FAILURE);
}

void installSignalHandlers()
{
    std::signal(SIGABRT, signal_handler);
    std::signal(SIGBUS, signal_handler);
    std::signal(SIGILL, signal_handler);
    std::signal(SIGINT, signal_handler);
    std::signal(SIGPIPE, signal_handler);
    std::signal(SIGSEGV, signal_handler);
}
#else
void installSignalHandlers()
{
}
#endif

void installStackTracePrintingSignalHandler()
{
    static const bool unused = (installSignalHandlers(), true);
    UNUSED(unused);
}

} // namespace utils
} // namespace idl
} // namespace maps
} // namespace yandex
