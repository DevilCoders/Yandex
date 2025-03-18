#include "blocking_counter.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/thread/pool.h>

using NThreading::TBlockingCounter;

Y_UNIT_TEST_SUITE(BlockingCounterTests) {
    Y_UNIT_TEST(Test) {
        const auto pool = CreateThreadPool(3, 0);
        const size_t count = 5;
        TBlockingCounter bc(count);
        for (size_t i = 0; i < count; ++i) {
            UNIT_ASSERT(pool->AddFunc([&] { bc.Decrement(); }));
        }
        bc.Wait();
    }
}
