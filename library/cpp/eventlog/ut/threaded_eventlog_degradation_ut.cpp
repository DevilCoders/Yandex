#include <library/cpp/eventlog/ut/test_events.ev.pb.h>

#include <library/cpp/eventlog/eventlog.h>
#include <library/cpp/eventlog/evdecoder.h>
#include <library/cpp/eventlog/iterator.h>
#include <library/cpp/eventlog/threaded_eventlog.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/algorithm.h>
#include <util/random/random.h>
#include <util/stream/file.h>
#include <util/string/cast.h>
#include <util/system/tempfile.h>
#include <util/thread/pool.h>

#include <atomic>

namespace {
    constexpr size_t NUM_WORKERS = 32;
    constexpr size_t NUM_EVENTS = 10000;
    constexpr size_t NUM_THREADS = 4;
    constexpr size_t QUEUE_SIZE = 1000;

    enum class EDegradationPolicy {
        Random,
        Disable,
    };

    enum class EOverflowPolicy {
        Drop,
        SyncWrite,
    };

    void RunTest(float dropTreshold, EDegradationPolicy degradationPolicy, EOverflowPolicy overflowPolicy) {
        SetRandomSeed(0x7ab95a8d);

        TTempFile tempFile{"event.log"};
        auto tempFileName = tempFile.Name();

        std::atomic<size_t> droppedCount = 0;
        {
            TEventLog eventLog(tempFileName, NEvClass::Factory()->CurrentFormat());
            TThreadedEventLog threadedEventLog(eventLog, NUM_THREADS, QUEUE_SIZE, [&](auto& wrapper) {
                switch (overflowPolicy) {
                case EOverflowPolicy::Drop:
                    droppedCount.fetch_add(1);
                    break;
                case EOverflowPolicy::SyncWrite:
                    wrapper.WriteFrame();
                    break;
                default:
                    Y_UNREACHABLE();
                }
            }, [&](float) {
                if (degradationPolicy == EDegradationPolicy::Random && RandomNumber<float>() < dropTreshold) {
                    droppedCount.fetch_add(1);
                    return TThreadedEventLog::EDegradationResult::ShouldDrop;
                }
                return TThreadedEventLog::EDegradationResult::ShouldWrite;
            });

            THolder<IThreadPool> pool = CreateThreadPool(NUM_WORKERS);

            for (size_t iter = 0; iter < NUM_WORKERS; ++iter) {
                pool->SafeAddFunc([&, iter] {
                    for (size_t i = iter * NUM_EVENTS; i < (iter + 1) * NUM_EVENTS; ++i) {
                        TSelfFlushLogFrame frame(threadedEventLog);
                        frame.LogEvent(TOneField(ToString(i)));
                    }
                });
            }

            pool->Stop();
        }

        size_t writtenCounter = 0;
        auto eventlog = NEventLog::CreateIterator(NEventLog::TOptions{}.SetFileName(tempFileName));
        for (const TEvent& ev : *eventlog) {
            if (ev.Class == TOneField::ID) {
                writtenCounter++;
            }
        }

        const size_t totalCount = NUM_WORKERS * NUM_EVENTS;
        UNIT_ASSERT_VALUES_EQUAL(totalCount, droppedCount.load() + writtenCounter);
        if (overflowPolicy == EOverflowPolicy::SyncWrite) {
            const size_t expectedDroppedCount = degradationPolicy == EDegradationPolicy::Random ? totalCount * dropTreshold : 0;
            UNIT_ASSERT(expectedDroppedCount * 0.9 <= droppedCount.load() && droppedCount.load() <= expectedDroppedCount * 1.1);
        }
    }

}

Y_UNIT_TEST_SUITE(ThreadedEventLogDegradation) {
    Y_UNIT_TEST(Degradation) {
        RunTest(0.1f, EDegradationPolicy::Random, EOverflowPolicy::SyncWrite);
        RunTest(0.5f, EDegradationPolicy::Random, EOverflowPolicy::SyncWrite);
        RunTest(0.9f, EDegradationPolicy::Random, EOverflowPolicy::SyncWrite);
    }

    Y_UNIT_TEST(NoDegradation) {
        RunTest(0.0f, EDegradationPolicy::Disable, EOverflowPolicy::SyncWrite);
        RunTest(1.0f, EDegradationPolicy::Disable, EOverflowPolicy::SyncWrite);
    }

    Y_UNIT_TEST(DropOnOverflow) {
        RunTest(0.0f, EDegradationPolicy::Disable, EOverflowPolicy::Drop);
        RunTest(1.0f, EDegradationPolicy::Disable, EOverflowPolicy::Drop);
    }
}
