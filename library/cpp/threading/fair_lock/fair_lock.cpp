#include "fair_lock.h"

#include <util/system/condvar.h>
#include <util/system/mutex.h>
#include <util/system/thread.h>
#include <util/system/yassert.h>
#include <util/generic/intrlist.h>

namespace {
    using TLock = TGuard<TMutex>;

    struct TWaitTicket: public TIntrusiveListItem<TWaitTicket> {
        TCondVar Cond;
    };

}

// TImpl

class TRecursiveFairLock::TImpl {
private:
    TMutex Lock;
    TIntrusiveList<TWaitTicket> Tickets;
    int LockCount;
    TThread::TId OwnerThreadId;

public:
    inline TImpl()
        : LockCount(0)
        , OwnerThreadId(TThread::ImpossibleThreadId())
    {
    }

    ~TImpl() {
    }

    void Acquire() noexcept {
        Acquire(/*wait*/ true);
    }

    bool TryAcquire() noexcept {
        return Acquire(/*wait*/ false);
    }

    void Release() noexcept {
        TLock lock(Lock);

        auto threadId = TThread::CurrentThreadId();
        Y_VERIFY(OwnerThreadId == threadId,
                 "Invalid lock usage, attempt to release locked by %" PRISZT " in %" PRISZT " thread", OwnerThreadId, threadId);
        Y_ASSERT(LockCount > 0);

        --LockCount;
        if (LockCount > 0) {
            // recursive use
            return;
        }

        OwnerThreadId = TThread::ImpossibleThreadId();

        if (!Tickets.Empty()) {
            Tickets.Begin()->Cond.Signal();
        }
    }

    bool IsFree() noexcept {
        TLock lock(Lock);
        return IsFreeInternal(lock);
    }

    bool IsOwnedByMe() noexcept {
        TLock lock(Lock);
        return LockCount > 0 && OwnerThreadId == TThread::CurrentThreadId();
    }

private:
    bool Acquire(bool wait) noexcept {
        TLock lock(Lock);

        auto threadId = TThread::CurrentThreadId();

        if (IsFreeInternal(lock)) {
            DoAcquire(threadId);
            return true;
        }

        if (OwnerThreadId == threadId) {
            // recursive use
            ++LockCount;
            return true;
        }

        if (wait) {
            // recursive wait is impossible
            TWaitTicket ticket;
            Tickets.PushBack(&ticket);
            do {
                ticket.Cond.WaitI(Lock);
            } while (LockCount != 0);

            DoAcquire(threadId);
            return true;
        }

        return false;
    }

    void DoAcquire(TThread::TId threadId) noexcept {
        Y_ASSERT(LockCount + 1 > 0); // check overflow
        ++LockCount;
        OwnerThreadId = threadId;
    }

    bool IsFreeInternal(TLock& /*lock*/) noexcept {
        return LockCount == 0 && Tickets.Empty();
    }
};

// TRecursiveFairLock

TRecursiveFairLock::TRecursiveFairLock()
    : Impl(new TImpl())
{
}

TRecursiveFairLock::~TRecursiveFairLock() {
}

void TRecursiveFairLock::Acquire() noexcept {
    Impl->Acquire();
}

bool TRecursiveFairLock::TryAcquire() noexcept {
    return Impl->TryAcquire();
}

void TRecursiveFairLock::Release() noexcept {
    Impl->Release();
}

bool TRecursiveFairLock::IsFree() noexcept {
    return Impl->IsFree();
}

bool TRecursiveFairLock::IsOwnedByMe() noexcept {
    return Impl->IsOwnedByMe();
}
