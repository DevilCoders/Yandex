#include "app.h"

#include <cloud/storage/core/libs/common/app.h>

#include <library/cpp/logger/backend.h>
#include <library/cpp/sighandler/async_signals_handler.h>

#include <cstdio>

namespace NCloud::NFileStore::NServer {

namespace {

////////////////////////////////////////////////////////////////////////////////

void ProcessSignal(int signum)
{
    if (signum == SIGINT || signum == SIGTERM) {
        AppStop(0);
    }
}

void ProcessAsyncSignal(int signum)
{
    if (signum == SIGHUP) {
        TLogBackend::ReopenAllBackends();
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void ConfigureSignals()
{
    std::set_new_handler(abort);

    // make sure that errors can be seen by everybody :)
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);

    // mask signals
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, ProcessSignal);
    signal(SIGTERM, ProcessSignal);

    SetAsyncSignalHandler(SIGHUP, ProcessAsyncSignal);
}

}   // namespace NCloud::NFileStore::NServer
