#include "async_task_batch.h"

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/thread/pool.h>
#include <util/datetime/base.h>
#include <util/random/random.h>
#include <util/datetime/cputimer.h>

Y_UNIT_TEST_SUITE(TAsyncTaskBatchTest) {
    struct TEnv {
    public:
        TEnv(size_t threadCount, size_t queueSize)
            : Batch(threadCount > 0 ? &Queue : nullptr)
        {
            if (threadCount) {
                Queue.Start(threadCount, queueSize);
            }
        }

        void AddTasks(const size_t count, TDuration sleep = {});

    public:
        TThreadPool Queue;
        TAsyncTaskBatch Batch;

        TAtomic Finished = 0;
    };

    class TTask
        : public TAsyncTaskBatch::ITask
    {
    public:
        explicit TTask(TEnv& env, TDuration sleep = {})
            : Env_(env)
            , Sleep_(sleep)
        {
        }

        void Process() override {
            if (Sleep_ != TDuration::Zero()) {
                Sleep(Sleep_);
            }
            AtomicIncrement(Env_.Finished);
        }
    private:
        TEnv& Env_;
        TDuration Sleep_;
    };

    void TEnv::AddTasks(const size_t count, TDuration sleep) {
        for (size_t i = 0; i < count; ++i) {
            Batch.Add(new TTask(*this, sleep));
        }
    }


    Y_UNIT_TEST(TestEmpty) {
        TEnv env(10, 1000);

        const auto info = env.Batch.WaitAllAndProcessNotStarted();
        UNIT_ASSERT(AtomicGet(env.Finished) == 0);
        UNIT_ASSERT(info.TotalTasks == 0);
        UNIT_ASSERT(info.SelfProcessedTasks == 0);
    }

    Y_UNIT_TEST(TestExecution) {
        TEnv env(10, 1000);

        const size_t N = 100;
        env.AddTasks(N);

        const auto info = env.Batch.WaitAllAndProcessNotStarted();
        UNIT_ASSERT(AtomicGet(env.Finished) == N);
        UNIT_ASSERT(info.TotalTasks == N);
    }

    Y_UNIT_TEST(TestRandom) {
        TEnv env(10, 1000);

        const size_t N = 1000;
        for (size_t i = 0; i < N; ++i) {
            env.Batch.Add(new TTask(env, TDuration::MicroSeconds(RandomNumber(200u))));
            Sleep(TDuration::MicroSeconds(RandomNumber(100u)));
        }

        const auto info = env.Batch.WaitAllAndProcessNotStarted();
        UNIT_ASSERT(AtomicGet(env.Finished) == N);
        UNIT_ASSERT(info.TotalTasks == N);
    }

    Y_UNIT_TEST(TestDummyWait) {
        TEnv env(10, 1000);

        const size_t N = 100;
        env.AddTasks(N);

        TInstant start = Now();
        while (AtomicGet(env.Finished) != N && Now() - start < TDuration::Seconds(1)) {
            Sleep(TDuration::MilliSeconds(50));
        }

        UNIT_ASSERT(AtomicGet(env.Finished) == N);

        const auto info = env.Batch.WaitAllAndProcessNotStarted();
        UNIT_ASSERT(info.TotalTasks == N);
        UNIT_ASSERT(info.SelfProcessedTasks == 0);
    }

    Y_UNIT_TEST(TestSmallQueue) {
        TEnv env(5, 5);

        const size_t N = 20;
        env.AddTasks(N, TDuration::MilliSeconds(10));

        const auto info = env.Batch.WaitAllAndProcessNotStarted();
        UNIT_ASSERT(AtomicGet(env.Finished) == N);
        UNIT_ASSERT(info.TotalTasks == N);
        UNIT_ASSERT(info.SelfProcessedTasks > 0);
        UNIT_ASSERT(info.SelfProcessedTasks <= N);
    }

    Y_UNIT_TEST(TestNoQueue) {
        TEnv env(0, 0);
        const size_t N = 10;
        env.AddTasks(N);

        const auto info = env.Batch.WaitAllAndProcessNotStarted();
        UNIT_ASSERT(AtomicGet(env.Finished) == N);
        UNIT_ASSERT(info.TotalTasks == N);
        UNIT_ASSERT(info.SelfProcessedTasks == N);
    }


    Y_UNIT_TEST(TestNoWait) {
        {
            TEnv env(0, 0);
            const size_t N = 10;
            env.AddTasks(N);
        }
        UNIT_ASSERT(true); //NOTE: in bug case deadlock occures in destructor, so this line is unreachable
    }

    Y_UNIT_TEST(TestExceptionFromTask) {
        TEnv env(5, 5);

        env.Batch.Add([] {
                throw yexception() << "Exception in batch task" << Endl;
            });

        const size_t N = 2;
        env.AddTasks(N, TDuration::MilliSeconds(10));

        const auto info = env.Batch.WaitAllAndProcessNotStarted();
        UNIT_ASSERT(AtomicGet(env.Finished) == N);
        UNIT_ASSERT(info.TotalTasks == N + 1);
    }

    Y_UNIT_TEST(TestExceptionFromTaskNoQueue) {
        TEnv env(0, 0);

        env.Batch.Add([] {
                throw yexception() << "Exception in batch task" << Endl;
            });

        const size_t N = 2;
        env.AddTasks(N, TDuration::MilliSeconds(10));

        const auto info = env.Batch.WaitAllAndProcessNotStarted();
        UNIT_ASSERT(AtomicGet(env.Finished) == N);
        UNIT_ASSERT(info.TotalTasks == N + 1);
    }

    Y_UNIT_TEST(TestExceptionFromTaskInWaitInfo) {
        TEnv env(5, 5);

        env.Batch.Add([] {
                throw yexception() << "Exception in batch task";
            });

        const size_t N = 2;
        env.AddTasks(N, TDuration::MilliSeconds(10));

        const auto info = env.Batch.WaitAllAndProcessNotStarted();
        UNIT_ASSERT(info.ExceptionPtr);
        try {
            std::rethrow_exception(info.ExceptionPtr);
        } catch (yexception& e) {
            Cout << e.what() << Endl;
            UNIT_ASSERT_STRINGS_EQUAL(e.what(), "Exception in batch task");
        }

        env.AddTasks(N, TDuration::MilliSeconds(10));

        const auto info1 = env.Batch.WaitAllAndProcessNotStarted();
        UNIT_ASSERT(!info1.ExceptionPtr);
    }

    Y_UNIT_TEST(TestConcurrency) {
        TEnv env(10, 10);

        std::function<void(size_t)> f;
        f = [&] (size_t x) {
            if (x > 1) {
                env.Batch.Add([&f, x] { f(x >> 1); });
                env.Batch.Add([&f, x] { f(x >> 1); });
            }
        };

        for (size_t n : {1, 32, 60, 1000}) {
            size_t ans = n;
            while (ans & (ans - 1)) {
                ans &= ans - 1;
            }
            ans = (ans << 1) - 1;

            env.Batch.Add([&] { f(n); }); // O(n)

            const auto info = env.Batch.WaitAllAndProcessNotStarted();
            UNIT_ASSERT(info.TotalTasks == ans);
        }
    }

    Y_UNIT_TEST(TestSummaryTime) {
        TEnv env(10, 10);

        constexpr TDuration dur = TDuration::MicroSeconds(10000);
        for (size_t n : { 1, 32, 60, 1000 }) {
            env.AddTasks(n, dur);
            const auto info = env.Batch.WaitAllAndProcessNotStarted();
            UNIT_ASSERT(info.TotalTasks == n);
            TDuration expected = n * dur;
            TDuration actual = info.GetTotalDuration();
            UNIT_ASSERT(static_cast<double>(expected.MicroSeconds()) * 1.02 >= static_cast<double>(actual.MicroSeconds()));
        }
    }
}
