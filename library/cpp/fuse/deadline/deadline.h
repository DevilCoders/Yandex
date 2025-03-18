#pragma once

#include <library/cpp/containers/intrusive_rb_tree/rb_tree.h>
#include <library/cpp/threading/future/future.h>

#include <util/datetime/base.h>
#include <util/generic/ptr.h>
#include <util/generic/set.h>
#include <util/system/mutex.h>
#include <util/system/thread.h>
#include <util/thread/factory.h>
#include <util/thread/pool.h>

namespace NThreading {

using TDeadlineOwner = intptr_t;

struct TDeadlineActionCompare {
    template <class T>
    static inline bool Compare(const T& a, const T& b) {
        return a.GetDeadline() < b.GetDeadline()
            || (a.GetDeadline() == b.GetDeadline() && &a < &b);
    }
};

class IDeadlineAction
    : public TThrRefBase
    , public TRbTreeItem<IDeadlineAction, TDeadlineActionCompare>
{
public:
    explicit IDeadlineAction(TInstant deadline)
        : Deadline_(deadline)
    { }

    TInstant GetDeadline() const {
        return Deadline_;
    }

    virtual void Timeout() = 0;
    virtual TDeadlineOwner GetOwner() const = 0;

private:
    const TInstant Deadline_;
};

using IDeadlineActionRef = TIntrusivePtr<IDeadlineAction>;

class TDeadlineScheduler : IThreadFactory::IThreadAble {
public:
    TDeadlineScheduler(TDuration pollInterval);
    ~TDeadlineScheduler();

    // Returns all active deadline actions that has not been fired.
    // Stop is a bit racy, caller must ensure that nothig will be
    // scheduled or removed at the time it calls Stop()
    TVector<IDeadlineActionRef> Stop();

    TVector<IDeadlineActionRef> Stop(TDeadlineOwner owner);

    void Schedule(IDeadlineActionRef action);
    void Remove(IDeadlineActionRef action);

    NThreading::TFuture<void> ScheduledFuture(TInstant deadline, TDeadlineOwner owner = 0);

private:
    void DoExecute();

private:
    const TDuration PollInterval_;
    THolder<IThreadFactory::IThread> T_;
    std::atomic<TThread::TId> SelfId_;
    TAtomic Stop_;
    TMutex Lock_;
    TRbTree<IDeadlineAction, TDeadlineActionCompare> DeadlineQueue_;
};

} // namespace NThreading
