#include "queue.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/system/event.h>

Y_UNIT_TEST_SUITE(MtpQueueSuite) {
    Y_UNIT_TEST(Counted) {
        TCountedMtpQueue queue;
        queue.Start(42);
        UNIT_ASSERT_VALUES_EQUAL(queue.GetBusyThreadsCount(), 0);
        UNIT_ASSERT_VALUES_EQUAL(queue.GetFreeThreadsCount(), 42);

        TAutoEvent start;
        TAutoEvent stop;
        UNIT_ASSERT(queue.AddFunc([&start, &stop] {
            start.Signal();
            stop.WaitI();
        }));
        start.WaitI();
        UNIT_ASSERT_VALUES_EQUAL(queue.GetBusyThreadsCount(), 1);
        UNIT_ASSERT_VALUES_EQUAL(queue.GetFreeThreadsCount(), 41);
        stop.Signal();
        queue.Stop();
    }
}
