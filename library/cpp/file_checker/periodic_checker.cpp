#include "periodic_checker.h"

#include <library/cpp/deprecated/atomic/atomic.h>

namespace NChecker {
    TPeriodicChecker::TPeriodicChecker(IUpdater& tgt, ui64 timerms)
        : Thr(&Check, this)
        , Ev(TSystemEvent::rAuto)
        , Tgt(tgt)
        , CheckerState(CS_IDLE)
    {
        SetTimerMS(timerms);
    }

    void TPeriodicChecker::Start() {
        if (CS_IDLE != CheckerState.load())
            return;

        CheckerState.store(CS_STARTING);
        Tgt.OnBeforeStart();
        Thr.Start();
        CheckerState.store(CS_RUNNING);
    }

    void TPeriodicChecker::Wake() {
        Ev.Signal();
    }

    void TPeriodicChecker::Stop() {
        if (CS_RUNNING != CheckerState.load())
            return;

        CheckerState.store(CS_STOPPING);
        Wake();
        Thr.Join();
        CheckerState.store(CS_STOPPED);
    }

    void* TPeriodicChecker::Check(void* p) {
        TPeriodicChecker* me = (TPeriodicChecker*)p;

        while (CS_STARTING == me->CheckerState.load())
            me->Ev.Wait(me->Timer);

        while (CS_RUNNING == me->CheckerState.load()) {
            if (me->Tgt.IsChanged() && !me->Tgt.Update())
                break;

            if (!me->Tgt.OnCheck())
                break;

            me->Ev.Wait(me->Timer);
        }

        return nullptr;
    }

}
