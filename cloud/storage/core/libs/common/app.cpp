#include "app.h"

#include "thread.h"

#include <library/cpp/actors/util/should_continue.h>

#include <util/datetime/base.h>
#include <util/generic/singleton.h>
#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/system/condvar.h>
#include <util/system/mutex.h>

namespace NCloud {

namespace {

////////////////////////////////////////////////////////////////////////////////

const TDuration ShouldContinueSleepTime = TDuration::Seconds(5);

////////////////////////////////////////////////////////////////////////////////

class TMainThread
{
private:
    TMutex WaitMutex;
    TCondVar WaitCondVar;

    static const TAtomicBase Init = 0;
    static const TAtomicBase Started = 1;
    static const TAtomicBase Stopped = 2;

    TProgramShouldContinue* ShouldContinue = nullptr;
    TAtomic State = Init;

public:
    static TMainThread* GetInstance()
    {
        return Singleton<TMainThread>();
    }

    int Run(TProgramShouldContinue& shouldContinue)
    {
        ShouldContinue = &shouldContinue;

        if (AtomicCas(&State, Started, Init)) {
            SetCurrentThreadName("Main");

            with_lock (WaitMutex) {
                while (ShouldContinue->PollState() == TProgramShouldContinue::Continue) {
                    WaitCondVar.WaitT(WaitMutex, ShouldContinueSleepTime);
                }
            }
        }

        return ShouldContinue->GetReturnCode();
    }

    void Stop(int exitCode)
    {
        if (AtomicCas(&State, Stopped, Started)) {
            ShouldContinue->ShouldStop(exitCode);
            WaitCondVar.Signal();
        }
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

int AppMain(TProgramShouldContinue& shouldContinue)
{
    return TMainThread::GetInstance()->Run(shouldContinue);
}

void AppStop(int exitCode)
{
    TMainThread::GetInstance()->Stop(exitCode);
}

}   // namespace NCloud
