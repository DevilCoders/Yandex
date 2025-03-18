#include <library/cpp/testing/unittest/registar.h>

#include "data_process_queue.h"

#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/thread/pool.h>

namespace NAntiRobot {

Y_UNIT_TEST_SUITE(DataProcessQueue) {

Y_UNIT_TEST(Works) {
    TAtomic counter = 0;
    const int count = 100;
    const int expectedSum = (1 + count) * count / 2;
    {
        TAtomicSharedPtr<IThreadPool> slave = CreateThreadPool(5);
        TDataProcessQueue<int> queue(slave, [&counter](int value) {
            AtomicAdd(counter, value);
        });

        for (int i = 1; i <= count; ++i) {
            UNIT_ASSERT(queue.Add(i));
        }
    }
    UNIT_ASSERT_VALUES_EQUAL(AtomicGet(counter), expectedSum);
}

Y_UNIT_TEST(GetProcessorDoesTheSameThing) {
    int valueOne, valueTwo;
    {
        TAtomicSharedPtr<IThreadPool> slave = CreateThreadPool(1);
        TDataProcessQueue<int*> queue(slave, [](int* value) {
            *value = 5;
        });
        UNIT_ASSERT(queue.Add(&valueOne));

        queue.GetProcessor()(&valueTwo);
    }
    UNIT_ASSERT_VALUES_EQUAL(valueOne, valueTwo);
}

}

}
