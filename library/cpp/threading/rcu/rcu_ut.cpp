#include "rcu.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/thread/pool.h>
#include <util/system/mutex.h>
#include <util/generic/algorithm.h>
#include <util/generic/xrange.h>

namespace NThreading {
    Y_UNIT_TEST_SUITE(Rcu) {
        Y_UNIT_TEST(UpdateWithValue) {
            TRcu<int> rcu;

            for (int i = 0; i < 100; ++i) {
                rcu.UpdateWithValue(i).Wait();
                UNIT_ASSERT_VALUES_EQUAL(i, *rcu.GetCopy());
            }
        }

        Y_UNIT_TEST(UpdateWithFunction) {
            TRcu<int> rcu(1);

            for (int i = 1; i <= 10; ++i) {
                rcu.UpdateWithFunction([=](int& value) {
                       value *= 2;
                   })
                    .Wait();
                UNIT_ASSERT_VALUES_EQUAL(*rcu.GetCopy(), 1 << i);
            }
        }

        void TestConcurrency(int producerThreads, int consumerThreads) {
            auto queue = MakeAtomicShared<TThreadPool>();
            queue->Start(consumerThreads);

            TRcu<TVector<int>> rcu(queue);

            const int updateCount = 1000;
            {
                auto producers = MakeHolder<TThreadPool>();
                producers->Start(producerThreads);

                for (int i = 0; i < updateCount; ++i) {
                    producers->SafeAddFunc([&]() {
                        rcu.UpdateWithFunction([](TVector<int>& data) {
                            data.push_back(0);
                        });
                    });
                }
            }

            rcu.UpdateWithFunction([](TVector<int>&) {
               })
                .Wait();

            UNIT_ASSERT_VALUES_EQUAL(rcu.GetCopy()->ysize(), updateCount);
        }

        Y_UNIT_TEST(SingleUpdaterSingleExecutor) {
            TestConcurrency(1, 1);
        }

        Y_UNIT_TEST(SingleUpdaterMultipleExecutors) {
            TestConcurrency(1, 5);
        }

        Y_UNIT_TEST(MultipleUpdatersMultipleExecutors) {
            TestConcurrency(6, 3);
        }

        Y_UNIT_TEST(UpdateWithFunctionReturnType) {
            // Compileability test
            TRcu<int> rcu;
            TFuture<void> f1 = rcu.UpdateWithValue(5);
            TFuture<void> f2 = rcu.UpdateWithFunction([](int& value) {
                ++value;
            });
            TFuture<int> f3 = rcu.UpdateWithFunction([](int&) { return 42; });
            TFuture<TVector<int>> f4 = rcu.UpdateWithFunction([](int&) {
                return TVector<int>();
            });
        }

        Y_UNIT_TEST(UpdateTasksGetDeletedWhenDone) {
            auto aliveTaskCounter = MakeAtomicShared<int>();

            {
                TRcu<int> rcu(0, CreateThreadPool(2));
                auto updatePostQueue = CreateThreadPool(2);

                for (int i = 0; i < 100; ++i) {
                    updatePostQueue->SafeAddFunc([=, &rcu]() {
                        rcu.UpdateWithFunction([aliveTaskCounter](int&) {
                            UNIT_ASSERT(aliveTaskCounter.RefCount() > 1);
                        });
                    });
                }
            }

            UNIT_ASSERT_VALUES_EQUAL(aliveTaskCounter.RefCount(), 1);
        }

        Y_UNIT_TEST(UpdatesAreAppliedInTheGivenOrder) {
            TRcu<TVector<int>> rcu(CreateThreadPool(3));

            const int updateCount = 1000;
            for (int i : xrange(updateCount)) {
                rcu.UpdateWithFunction([i](TVector<int>& data) {
                    data.push_back(i);
                });
            }

            rcu.UpdateWithFunction([](TVector<int>&) {
               })
                .Wait();

            UNIT_ASSERT_VALUES_EQUAL(rcu.GetCopy()->ysize(), updateCount);
            auto numbers = *rcu.GetCopy();
            UNIT_ASSERT(IsSorted(numbers.begin(), numbers.end()));
        }

        struct TFailAddQueue: public TThreadPool {
            bool Add(IObjectInQueue*) override {
                return false;
            }
        };

        Y_UNIT_TEST(UpdatesAreFailedIfQueueIsFull) {
            TVector<TFuture<void>> updateResult;

            {
                TRcu<int> rcu(new TFailAddQueue);
                for (int i : xrange(100)) {
                    Y_UNUSED(i);
                    updateResult.push_back(rcu.UpdateWithValue(5));
                }
            }

            UNIT_ASSERT(AllOf(updateResult, [](TFuture<void> f) { return f.HasException(); }));
        }

        Y_UNIT_TEST(NoCrashIfLongChainOfUpdatesIsCancelled) {
            auto queue = MakeAtomicShared<TThreadPool>();
            queue->Start(3);

            TRcu<TVector<int>> rcu(queue);
            TAtomic startUpdates = false;

            for (int i : xrange(40000)) {
                rcu.UpdateWithFunction([i, &startUpdates](TVector<int>& data) {
                    while (!AtomicGet(startUpdates)) {
                    }
                    data.push_back(i);
                });
            }

            AtomicSet(startUpdates, true);
            queue->Stop();
        }

        Y_UNIT_TEST(IfUpdateThrowsNextUpdatesAreApplied) {
            TRcu<int> rcu(0);
            TAtomic startUpdates = false;

            TFuture<void> failedUpdate = rcu.UpdateWithFunction([&startUpdates](int&) {
                while (!AtomicGet(startUpdates)) {
                }
                ythrow yexception();
            });

            TFuture<void> successfulUpdate = rcu.UpdateWithValue(5);

            UNIT_ASSERT(!failedUpdate.HasException() && !failedUpdate.HasValue());
            AtomicSet(startUpdates, true);

            successfulUpdate.Wait();
            UNIT_ASSERT(failedUpdate.HasException());
            UNIT_ASSERT(successfulUpdate.HasValue());
            UNIT_ASSERT_VALUES_EQUAL(*rcu.GetCopy(), 5);
        }
    }

}
