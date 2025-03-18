#include "deadline.h"

#include <util/generic/vector.h>

namespace NThreading {

namespace {

class TDeadlineFuture : public IDeadlineAction {
public:
    TDeadlineFuture(TInstant deadline, TDeadlineOwner owner)
        : IDeadlineAction(deadline)
        , Owner_(owner)
        , Promise_(NThreading::NewPromise())
    {
    }

    ~TDeadlineFuture() override {
        Promise_.TrySetException(std::make_exception_ptr(
            yexception() << "destroyed before timeout"
        ));
    }

    void Timeout() override {
        Promise_.SetValue();
    }

    TDeadlineOwner GetOwner() const override {
        return Owner_;
    }

    NThreading::TFuture<void> GetFuture() {
        return Promise_.GetFuture();
    }

private:
    const TDeadlineOwner Owner_;
    NThreading::TPromise<void> Promise_;
};

} // namespace

TDeadlineScheduler::TDeadlineScheduler(TDuration pollInterval)
    : PollInterval_(pollInterval)
    , SelfId_(TThread::ImpossibleThreadId())
    , Stop_(0)
{
    T_ = SystemThreadFactory()->Run(this);
}

TDeadlineScheduler::~TDeadlineScheduler() {
    Stop();
}

void TDeadlineScheduler::Schedule(IDeadlineActionRef action) {
    Y_ASSERT(action->ParentTree() == nullptr);
    if (action->GetDeadline() != TInstant::Max()) {
        with_lock(Lock_) {
            action->Ref();
            DeadlineQueue_.Insert(*action);
        }
    }
}

void TDeadlineScheduler::Remove(IDeadlineActionRef action) {
    if (action->GetDeadline() == TInstant::Max()) {
        Y_ASSERT(action->ParentTree() == nullptr);
        return;
    }

    if (action->ParentTree() != &DeadlineQueue_) {
        Y_ASSERT(false);
        return;
    }

    with_lock(Lock_) {
        if (action->ParentTree() == &DeadlineQueue_) {
            DeadlineQueue_.Erase(*action);
            action->UnRef();
        }
    }
}

TVector<IDeadlineActionRef> TDeadlineScheduler::Stop(TDeadlineOwner owner) {
    TVector<IDeadlineActionRef> res;
    auto end = MakeIntrusive<TDeadlineFuture>(TInstant(), 0);

    with_lock(Lock_) {
        auto it = DeadlineQueue_.Begin();
        while (it != DeadlineQueue_.End()) {
            if (it->GetOwner() == owner) {
                res.emplace_back(&*it);
                res.back()->UnRef();
                DeadlineQueue_.Erase(it++);
            } else {
                ++it;
            }
        }

        end->Ref();
        DeadlineQueue_.Insert(*end);
    }

    // Wait for all in-flight requests to end
    auto fut = end->GetFuture();
    // We need to drop TDeadlineFuture, so we do not deadlock if this scheduler
    // is concurrently fully stopped and TDeadlineFuture is removed from queue.
    // By dropping in this case we will trigger its destructor that will set
    // future with exception.
    end.Drop();
    fut.Wait();
    return res;
}

void TDeadlineScheduler::DoExecute() {
    SelfId_ = TThread::CurrentThreadId();
    TThread::SetCurrentThreadName("TDeadlineScheduler");
    TVector<IDeadlineActionRef> buf;

    while (!AtomicGet(Stop_)) {
        TInstant now = TInstant::Now();

        TInstant nextDeadline = TInstant::Max();

        with_lock(Lock_) {
            while (DeadlineQueue_) {
                auto it = DeadlineQueue_.Begin();
                auto curDeadline = it->GetDeadline();
                if (curDeadline <= now) {
                    buf.emplace_back(&*it);
                    buf.back()->UnRef();
                    DeadlineQueue_.Erase(it++);
                } else {
                    nextDeadline = curDeadline;
                    break;
                }
            }
        }

        for (auto& action : buf) {
            action->Timeout();
            action.Drop();
        }
        buf.clear();

        nextDeadline = Min(now + PollInterval_, nextDeadline);
        SleepUntil(nextDeadline);
    }
}

TVector<IDeadlineActionRef> TDeadlineScheduler::Stop() {
    TVector<IDeadlineActionRef> res;
    AtomicSet(Stop_, 1);

    Y_ASSERT(TThread::CurrentThreadId() != SelfId_);

    if (T_) {
        T_->Join();
    }

    with_lock(Lock_) {
        DeadlineQueue_.ForEachNoOrder([&](IDeadlineAction& deadline) {
            res.emplace_back(&deadline);
            deadline.UnRef();
        });
        DeadlineQueue_.Clear();

        return res;
    }
}

NThreading::TFuture<void> TDeadlineScheduler::ScheduledFuture(TInstant deadline, TDeadlineOwner owner) {
    auto fut = MakeIntrusive<TDeadlineFuture>(deadline, owner);
    Schedule(fut);
    return fut->GetFuture();
}

} // namespace NThreading
