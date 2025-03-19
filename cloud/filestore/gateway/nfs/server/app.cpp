#include "app.h"

#include "bootstrap.h"
#include "ganesha.h"
#include "options.h"

#include <library/cpp/deprecated/atomic/atomic.h>
#include <library/cpp/logger/backend.h>
#include <library/cpp/sighandler/async_signals_handler.h>

#include <util/generic/singleton.h>

namespace NCloud::NFileStore::NGateway {

namespace {

////////////////////////////////////////////////////////////////////////////////

class TMainThread
{
private:
    TAtomic ExitCode = 0;

public:
    static TMainThread* GetInstance()
    {
        return Singleton<TMainThread>();
    }

    int Run(TBootstrap& bootstrap)
    {
        auto options = bootstrap.GetOptions();

        int exitCode = nfs_ganesha_start(
            options->ConfigFile.c_str(),
            options->LogFile.c_str(),
            options->PidFile.c_str(),
            options->RecoveryDir.c_str());

        return exitCode ? exitCode : AtomicGet(ExitCode);
    }

    void Stop(int exitCode)
    {
        AtomicSet(ExitCode, exitCode);

        nfs_ganesha_stop();
    }
};

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

int AppMain(TBootstrap& bootstrap)
{
    return TMainThread::GetInstance()->Run(bootstrap);
}

void AppStop(int exitCode)
{
    TMainThread::GetInstance()->Stop(exitCode);
}

}   // namespace NCloud::NFileStore::NGateway
