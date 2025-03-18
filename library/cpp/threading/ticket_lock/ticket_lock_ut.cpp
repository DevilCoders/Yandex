#include "ticket_lock.h"

#include <library/cpp/testing/unittest/registar.h>

namespace NThreading {
    ////////////////////////////////////////////////////////////////////////////////

    Y_UNIT_TEST_SUITE(TTicketLock) {
        void TestLock() {
            TTicketLock lock;
            UNIT_ASSERT(!lock.IsLocked());

            lock.Acquire();
            UNIT_ASSERT(lock.IsLocked());

            lock.Release();
            UNIT_ASSERT(!lock.IsLocked());

            UNIT_ASSERT(lock.TryAcquire());
            UNIT_ASSERT(lock.IsLocked());

            UNIT_ASSERT(!lock.TryAcquire());
            UNIT_ASSERT(lock.IsLocked());

            lock.Release();
            UNIT_ASSERT(!lock.IsLocked());
        }
    }

}
