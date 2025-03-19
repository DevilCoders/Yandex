#pragma once

#include <library/cpp/threading/future/future.h>
#include <library/cpp/threading/hot_swap/hot_swap.h>

#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/system/mutex.h>
#include <util/system/event.h>
#include <util/generic/ptr.h>

namespace NUseTracker {

template<typename Func>
class Once {
private:
    Func func;
    TAtomic first;

public:
    Once(Func func)
        : func(func)
        , first(0)
    {
    }

    template<typename... Args>
    void operator()(Args&&... args) {
        if (AtomicIncrement(first) == 1) {
            func(std::forward<decltype(args)>(args)...);
        }
    }
};

template<typename Func>
auto DoOnce(Func f) {
    return Once<Func>(std::move(f));
}

template<typename Func>
class OnDestroy: public TAtomicRefCount<OnDestroy<Func>> {
private:
    Func action;
public:
    explicit OnDestroy(Func f)
        : action(std::move(f))
    {
    }

    void operator ()() {
        action();
    }

    ~OnDestroy() {
        action();
    }
};

template<typename Func>
TIntrusivePtr<OnDestroy<Func>> DoOnDestroy(Func f) {
    return MakeIntrusive<OnDestroy<Func>>(std::move(f));
}

// just explicit lambda
class TFutureSetter {
public:
    TFutureSetter(NThreading::TPromise<void> toSignal)
        : ToSignal(toSignal)
    {
    }

    void operator ()() {
        ToSignal.SetValue();
    }

private:
    NThreading::TPromise<void> ToSignal;
};

NThreading::TFuture<void> ShouldBeSet(
        NThreading::TFuture<void> future,
        TString message = "destroyed promise without value"
    );


class TUseTracker: TNonCopyable {
public:
    class TWaiter: public TAtomicRefCount<TWaiter> {
    public:
        TWaiter(TManualEvent& ev)
            : ToSignal(ev)
        {
        }

        ~TWaiter() {
            ToSignal.Signal();
        }

    private:
        TManualEvent& ToSignal;
    };

    using TWaiterRef = TIntrusivePtr<TWaiter>;

public:
    TUseTracker(bool stopped = false) {
        Guarded.Signal();
        if (!stopped) {
            Start();
        }
    }

    ~TUseTracker() {
        Stop();
    }

    TWaiterRef New() {
        return Waiter.AtomicLoad();
    }

    void Stop() {
        with_lock(Lock) {
            if (Waiter.AtomicLoad()) {
                Waiter.AtomicStore(nullptr);
                Guarded.WaitI();
            }
        }
    }

    void Start() {
        Stop();
        with_lock(Lock) {
            if (!Waiter.AtomicLoad()) {
                Guarded.Reset();
                Waiter.AtomicStore(new TWaiter(Guarded));
            }
        }
    }

    template<typename TFunc>
    auto Tracking(TFunc func) {
        return [this, func=std::forward<TFunc>(func)](auto&&... args) {
            if (auto waiter = New()) {
                func(std::move(waiter), std::forward<decltype(args)>(args)...);
            }
        };
    }

private:
    TMutex Lock;
    TManualEvent Guarded;
    THotSwap<TWaiter> Waiter;
};

}
