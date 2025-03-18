#include "mtp_queue_decorators.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/xrange.h>
#include <library/cpp/deprecated/atomic/atomic.h>

namespace NAntiRobot {

namespace {

struct TAlwaysFail : public IThreadPool {
    TAtomic Size_ = 0;
    bool IsStarted = false;

    bool Add(IObjectInQueue*) override {
        AtomicIncrement(Size_);
        return false;
    }

    void Start(size_t, size_t) override {
        IsStarted = true;
    }

    void Stop() noexcept override {
        IsStarted = false;
    }

    size_t Size() const noexcept override {
        return AtomicGet(Size_);
    }
};

}

Y_UNIT_TEST_SUITE(MtpQueueDecorator) {

Y_UNIT_TEST(DecoratesSlaveQueue) {
    auto mock = MakeAtomicShared<TAlwaysFail>();
    TFailCountingQueue queue(mock);

    queue.Start(1, 1);
    UNIT_ASSERT(mock->IsStarted);

    for (auto _ : xrange(10)) {
        Y_UNUSED(_);
        bool wasAdded = queue.AddFunc([]{});
        Y_UNUSED(wasAdded);
    }
    UNIT_ASSERT_VALUES_EQUAL(mock->Size(), 10);

    queue.Stop();
    UNIT_ASSERT(!mock->IsStarted);
}

}

Y_UNIT_TEST_SUITE(FailCountingQueue) {

Y_UNIT_TEST(ConcurrentlyCountsFails) {
    auto mock = MakeAtomicShared<TAlwaysFail>();
    TFailCountingQueue queue(mock);

    const int taskCount = 100;
    {
        auto workQueue = CreateThreadPool(5);
        for (auto _ : xrange(taskCount)) {
            Y_UNUSED(_);
            workQueue->SafeAddFunc([&] {
                bool wasAdded = queue.AddFunc([]{});
                Y_UNUSED(wasAdded);
            });
        }
    }

    UNIT_ASSERT_VALUES_EQUAL(queue.GetFailedAdditionsCount(), taskCount);
}

}

Y_UNIT_TEST_SUITE(RpsCountingQueue) {

Y_UNIT_TEST(ConcurrentlyCountsRps) {
    auto queue = CreateThreadPool(5);
    TRpsCountingQueue rpsCounter(std::move(queue));

    const int taskCount = 25;
    const TDuration taskDuration = TDuration::MilliSeconds(20);
    for (auto _ : xrange(taskCount)) {
        Y_UNUSED(_);
        rpsCounter.SafeAddFunc([=] {
            Sleep(taskDuration);
        });
    }
    rpsCounter.Stop();

    UNIT_ASSERT_VALUES_EQUAL(rpsCounter.GetRps().Processed, taskCount);
    UNIT_ASSERT(rpsCounter.GetRps().Time.GetValue() >= (taskDuration * taskCount).GetValue());
}

}

}
