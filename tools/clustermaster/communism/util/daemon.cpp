#include "dirut.h"
#include "file_reopener_by_signal.h"
#include "pidfile.h"

#include <util/generic/singleton.h>
#include <util/generic/yexception.h>
#include <util/system/backtrace.h>
#include <util/system/daemon.h>
#include <util/system/file.h>

#ifndef _win_

#include <errno.h>
#include <signal.h>

#endif // !_win_

#include "daemon.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void ReopenStdinToDevNull() {
#ifndef _win_
    TFileHandle devNull("/dev/null", RdOnly);

    if (!devNull.IsOpen()) {
        ythrow TSystemError() << "failed to open /dev/null"sv;
    }

    if (devNull == STDIN_FILENO) {
        return;
    }

    TFileHandle stdIn(STDIN_FILENO);
    const bool ret = stdIn.LinkTo(devNull);
    stdIn.Release();

    if (!ret) {
        ythrow TSystemError() << "failed to dup2"sv;
    }
#endif
}

void ReopenStdoutStderrToDevNull() {
#ifndef _win_
    TFileHandle devNull("/dev/null", WrOnly);

    if (!devNull.IsOpen()) {
        ythrow TSystemError() << "failed to open /dev/null"sv;
    }

    for (int fd = STDOUT_FILENO; fd <= STDERR_FILENO; ++fd) {
        TFileHandle out(fd);
        const bool ret = out.LinkTo(devNull);
        out.Release();

        if (!ret) {
            ythrow TSystemError() << "failed to dup2"sv;
        }
    }

#endif
}

static THolder<TFileReopenerBySignal> LogFileReopener;

void InitializeDaemonGeneric(
        const TString& logFileOption,
        const TString& pidFileOption,
        bool foreground)
{
#ifndef _win_
    signal(SIGPIPE, SIG_IGN);

    try {
        ReopenStdinToDevNull();
    } catch (const yexception& e) {
        Y_FAIL("%s", e.what());
    }

    if (!foreground)
        NDaemonMaker::CloseFrom(STDERR_FILENO + 1);

    // open pidfile
    if (!pidFileOption.empty()) {
        Singleton< THolder<TPidFile> >()->Reset(new TPidFile(RealPathFixed(pidFileOption)));
    }

    if (!logFileOption.empty()) {
        TString logFilePathReal = RealPathFixed(logFileOption);

        Cerr << "opening log to " << logFilePathReal << Endl;
        LogFileReopener.Reset(new TFileReopenerBySignal(logFilePathReal, true, true));
        Cerr << "log opened" << Endl;
    }

    // daemonize
    if (!foreground) {
        if (logFileOption.empty()) {
            Cerr << "Beware: log file is not specified" << Endl;
            Cerr << "Redirecting stdout and stderr to /dev/null" << Endl;
            Cerr << "This is the last message from this process" << Endl;
            ReopenStdoutStderrToDevNull();
        }

        if (daemon(1, 1) != 0) {
            ythrow yexception() << "daemon failed";
        }

        InstallSegvHandler();
    }

    // write final pid to the pidfile
    if (Singleton< THolder<TPidFile> >()->Get()) {
        (*Singleton< THolder<TPidFile> >())->Update();
        (*Singleton< THolder<TPidFile> >())->SetDeleteOnExit();
    }

#endif
}
