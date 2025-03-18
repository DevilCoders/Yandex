#pragma once

#include <util/system/event.h>
#include <util/system/thread.h>

#include <atomic>

namespace NChecker {
    class TPeriodicChecker;

    class IUpdater {
    public:
        virtual ~IUpdater() = default;

        // it's your responsibility to make sure other threads won't race with checker thread
        virtual bool Update() = 0;
        // same shit
        virtual bool OnCheck() {
            return true;
        }

        virtual bool IsChanged() const = 0;

        virtual void OnBeforeStart() {
        }
    };

    class TPeriodicChecker {
        enum ECheckerState {
            CS_IDLE,
            CS_STARTING,
            CS_RUNNING,
            CS_STOPPING,
            CS_STOPPED
        };

        TThread Thr;
        TSystemEvent Ev;
        IUpdater& Tgt;

        // assumed aligned assignments are atomic

        volatile ui64 Timer;

        std::atomic<ECheckerState> CheckerState;

    public:
        explicit TPeriodicChecker(IUpdater& tgt, ui64 timerms = 10000);

        ~TPeriodicChecker() {
            Stop();
        }

        void SetTimerMS(ui64 timer) {
            Timer = Max<ui64>(timer, 1);
        }

        void Start();
        void Wake();
        void Stop(); // do not forget to stop checker before dropping updater

    private:
        static void* Check(void* p);
    };

}
