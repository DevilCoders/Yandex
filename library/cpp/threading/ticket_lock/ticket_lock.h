#pragma once

#include <util/generic/noncopyable.h>
#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/system/spinlock.h>

namespace NThreading {
    ////////////////////////////////////////////////////////////////////////////////
    // Fair alternative to SpinLock.
    // It could be faster too as there is no CAS involved on fast path.

    class TTicketLock: private TNonCopyable {
    private:
        TAtomic In;
        TAtomic Out;

    public:
        TTicketLock()
            : In(0)
            , Out(0)
        {
        }

        void Acquire() {
            TAtomicBase ticket = AtomicIncrement(In) - 1;
            while (ticket != AtomicGet(Out)) {
                SpinLockPause();
            }
        }

        bool TryAcquire() {
            TAtomicBase ticket = AtomicGet(In);
            return ticket == AtomicGet(Out) && AtomicCas(&In, ticket + 1, ticket);
        }

        bool IsLocked() const {
            return AtomicGet(In) != AtomicGet(Out);
        }

        void Release() {
            AtomicSet(Out, Out + 1);
        }
    };

}
