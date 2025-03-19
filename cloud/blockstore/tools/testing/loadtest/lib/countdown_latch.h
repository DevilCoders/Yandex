#pragma once

#include "public.h"

#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/system/event.h>

namespace NCloud::NBlockStore::NLoadTest {

////////////////////////////////////////////////////////////////////////////////

class TCountDownLatch
{
private:
    TManualEvent Event;
    TAtomic Counter;

public:
    TCountDownLatch(TAtomicBase counter)
        : Counter(counter)
    {
    }

public:
    class TGuard: TNonCopyable
    {
    private:
        TCountDownLatch& Parent;

    public:
        TGuard(TCountDownLatch& parent)
            : Parent(parent)
        {
        }

        ~TGuard()
        {
            Parent.CountDown();
        }
    };

    auto Guard()
    {
        return TGuard(*this);
    }

    void CountDown()
    {
        auto result = AtomicDecrement(Counter);
        Y_VERIFY_DEBUG(result >= 0);
        if (result == 0) {
            Event.Signal();
        }
    }

    bool Done() const
    {
        return AtomicGet(Counter) == 0;
    }

    void Wait()
    {
        Event.WaitI();
    }
};

}   // namespace NCloud::NBlockStore::NLoadTest
