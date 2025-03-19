#include "periodic.h"

#include <kernel/searchlog/errorlog.h>

namespace NSearch {

    TPeriodicExecutor::TPeriodicExecutor(
        std::function<void()> worker,
        TDuration interval,
        std::function<void()> init
    )
        : Destroying_(0)
        , Worker_(std::move(worker))
        , Init_(std::move(init))
        , Interval_(interval)
        , Thread_(std::bind(&TPeriodicExecutor::ThreadFunc, this))
    {
    }

    TPeriodicExecutor::~TPeriodicExecutor() {
        SignalToStop();
    }

    void TPeriodicExecutor::ThreadFunc() {
        if (Init_) {
            try {
                Init_();
            } catch (...) {
                SEARCH_ERROR << CurrentExceptionMessage();
                abort();
            }
        }

        do {
            auto deadline = TInstant::Now() + Interval_;
            Waiter_.Reset();
            try {
                Worker_();
            } catch (...) {
                SEARCH_ERROR << "uncaught exception: " << ::CurrentExceptionMessage();
            }
            FirstRun_.Signal();
            Waiter_.WaitD(deadline);
        } while (AtomicGet(Destroying_) != 1);
    }
}
