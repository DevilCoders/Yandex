#include "fair_lock.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/system/thread.h>
#include <util/datetime/base.h>

Y_UNIT_TEST_SUITE(RecursiveFairLockTest) {
    Y_UNIT_TEST(Recursive) {
        TRecursiveFairLock lock;

        lock.Acquire();
        UNIT_ASSERT(!lock.IsFree());
        UNIT_ASSERT(lock.IsOwnedByMe());

        UNIT_ASSERT(lock.TryAcquire());
        UNIT_ASSERT(!lock.IsFree());
        UNIT_ASSERT(lock.IsOwnedByMe());

        lock.Acquire();
        UNIT_ASSERT(!lock.IsFree());
        UNIT_ASSERT(lock.IsOwnedByMe());

        lock.Release();
        UNIT_ASSERT(!lock.IsFree());
        UNIT_ASSERT(lock.IsOwnedByMe());

        lock.Release();
        UNIT_ASSERT(!lock.IsFree());
        UNIT_ASSERT(lock.IsOwnedByMe());

        lock.Release();
        UNIT_ASSERT(lock.IsFree());
        UNIT_ASSERT(!lock.IsOwnedByMe());
    }

    Y_UNIT_TEST(Complex) {
        struct TMyThread: public ISimpleThread {
            TRecursiveFairLock& Lock;
            TInstant Got;
            TInstant Release;

            TMyThread(TRecursiveFairLock& lock)
                : Lock(lock)
            {
            }

            void* ThreadProc() noexcept override {
                TGuard<TRecursiveFairLock> lock(Lock);
                Got = TInstant::Now();
                UNIT_ASSERT(!Lock.IsFree());
                UNIT_ASSERT(Lock.IsOwnedByMe());
                Sleep(TDuration::MilliSeconds(500));
                Release = TInstant::Now();
                return nullptr;
            }
        };

        TRecursiveFairLock lock;
        lock.Acquire();

        TMyThread t1(lock);
        TMyThread t2(lock);

        t1.Start();
        t2.Start();

        TInstant beforeRelease = TInstant::Now();

        lock.Release();

        t1.Join();
        t2.Join();

        UNIT_ASSERT(beforeRelease < t1.Got);
        UNIT_ASSERT(beforeRelease < t2.Got);

        if (t1.Got > t2.Got) {
            UNIT_ASSERT(t1.Release > t2.Release);
        } else {
            UNIT_ASSERT(t1.Release < t2.Release);
        }
    }
}
