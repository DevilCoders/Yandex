#include <util/system/platform.h>

#ifndef _win_

#include <signal.h>

#endif // _win_

#include "file_reopener_by_signal.h"

#include <util/generic/yexception.h>
#include <util/stream/output.h>

static TFileReopenerBySignal* current;

static void trampoline(int, siginfo_t*, void*) {
    // important to easier find out why log siddenly truncated
    Cerr << "got USR1, reopening log" << Endl;
    try {
        current->Reopen();
    } catch (...) {
        // it is better to do nothing than crash
        Cerr << CurrentExceptionMessage() << Endl;
    }
}

TFileReopenerBySignal::TFileReopenerBySignal(const TString& fileName, bool dupStdout, bool dupStderr)
    : FileName(fileName)
    , DupStdout(dupStdout)
    , DupStderr(dupStderr)
{
#ifndef _win_
    current = this;

    struct sigaction oldSa;

    struct sigaction sa;
    Zero(sa);
    sa.sa_sigaction = trampoline;
    sa.sa_flags = SA_SIGINFO;

    int r = sigaction(SIGUSR1, &sa, &oldSa);
    if (r < 0) {
        ythrow TSystemError() << "sigaction failed";
    }

    if (oldSa.sa_sigaction != nullptr
            && (void*) oldSa.sa_sigaction != SIG_DFL
            && (void*) oldSa.sa_sigaction != SIG_IGN)
    {
        ythrow yexception() << "signal handler is already installed";
    }
#endif // !_win_

    Reopen();
}

TFileReopenerBySignal::~TFileReopenerBySignal() {
    if (SIG_ERR != signal(SIGUSR1, SIG_DFL)) {
        // error, but we should not throw in destructor
    }
}

void TFileReopenerBySignal::Reopen() {
    // TODO: this code is not correct in signal handler
    TFile tmp(FileName, OpenAlways|WrOnly|Seq|ForAppend);
    if (File.IsOpen()) {
        File.Flush();
        File.LinkTo(tmp);
    } else {
        File = tmp;
    }

#ifndef _win_
    if (DupStdout) {
        if (dup2(File.GetHandle(), 1) < 0)
            ythrow TSystemError() << "dup2";
    }
    if (DupStderr) {
        if (dup2(File.GetHandle(), 2) < 0)
            ythrow TSystemError() << "dup2";
    }
#endif // !_win_
}

