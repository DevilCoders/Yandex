#include "bomb.h"
#include <library/cpp/balloc/optional/operators.h>
#include <library/cpp/logger/global/global.h>

namespace {
    const int BombExitCode = -128;
}

TBomb::TBomb(const TDuration& timeout, const TString& message)
    : Deadline(timeout.ToDeadLine())
    , Message(message)
{
    if (timeout.GetValue())
        Thread = SystemThreadFactory()->Run(this);
}

TBomb::~TBomb() {
    Deactivate();
}

void TBomb::Deactivate() {
    Deactivated.Signal();
    if (!!Thread)
        Thread->Join();
}

void TBomb::DoExecute() {
    ThreadDisableBalloc();
    if (!Deactivated.WaitD(Deadline)) {
        FATAL_LOG << "The process has been shut down by timeout " << Deadline << Endl;
        _exit(BombExitCode); // _exit ignores static runtime objects
    }
}
