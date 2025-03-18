#include "rt_graph.h"

#include <library/cpp/testing/unittest/registar.h>
#include <util/thread/pool.h>

namespace NComputeRTGraph {
    struct TInc {
        TInc(TAtomic& x, TDuration presleep = {})
            : X(x)
            , Presleep(presleep)
        {
        }

        void operator()() {
            Sleep(Presleep);
            AtomicAdd(X, 1);
        }

        TAtomic& X;
        TDuration Presleep;
    };

    Y_UNIT_TEST_SUITE(NComputeRTGraph) {
        Y_UNIT_TEST(StartStop) {
            TRTGraph graph(2);
            graph.Stop();
        }

        Y_UNIT_TEST(WaitTask) {
            TRTGraph graph(2);
            TAtomic x(0);
            auto t1 = graph.AddFunc(TInc(x));
            t1->Wait();
            UNIT_ASSERT(t1->IsFinished());
            UNIT_ASSERT_EQUAL(AtomicGet(x), 1);
            graph.Stop();
        }

        Y_UNIT_TEST(AddAfterCompletion) {
            TRTGraph graph(2);
            TAtomic x(0);
            auto t1 = graph.AddFunc(TInc(x));
            t1->Wait();
            auto t2 = graph.AddFunc(TInc(x), {t1});
            graph.Stop();
            UNIT_ASSERT_EQUAL(AtomicGet(x), 2);
        }

        Y_UNIT_TEST(SingleTask) {
            TRTGraph graph(2);
            TAtomic x(0);
            auto t1 = graph.AddFunc([&x]() {
                Sleep(TDuration::MilliSeconds(1000));
                AtomicAdd(x, 1);
            });
            graph.Stop();
            UNIT_ASSERT(t1->IsFinished());
            UNIT_ASSERT_EQUAL(AtomicGet(x), 1);
        }

        Y_UNIT_TEST(MultiTask) {
            TRTGraph graph(2);
            TAtomic x(0);
            graph.AddFunc(TInc(x));
            graph.AddFunc(TInc(x));
            graph.AddFunc(TInc(x));
            graph.AddFunc(TInc(x));
            graph.Stop();
            UNIT_ASSERT_EQUAL(AtomicGet(x), 4);
        }

        Y_UNIT_TEST(Queue) {
            TRTGraph graph(2);
            TAtomic x(0);
            auto t1 = graph.AddFunc([&x]() {
                Sleep(TDuration::MilliSeconds(100));
                AtomicAdd(x, 1);
            });
            auto t2 = graph.AddFunc(TInc(x), {t1});
            auto t3 = graph.AddFunc(TInc(x), {t2});
            auto t4 = graph.AddFunc(TInc(x), {t3});
            auto t5 = graph.AddFunc(TInc(x), {t4});
            graph.Stop();
            UNIT_ASSERT(t1->IsFinished());
            UNIT_ASSERT(t2->IsFinished());
            UNIT_ASSERT(t3->IsFinished());
            UNIT_ASSERT(t4->IsFinished());
            UNIT_ASSERT(t5->IsFinished());
            UNIT_ASSERT_EQUAL(AtomicGet(x), 5);
        }

        Y_UNIT_TEST(SeveralTaskGenerators) {
            TRTGraph graph(3);

            TAtomic x(0);

            auto queue = CreateThreadPool(20);
            for (int i = 0; i < 40; ++i) {
                queue->SafeAddFunc([&x, &graph]() {
                    auto t1 = graph.AddFunc([&x]() {
                        Sleep(TDuration::MilliSeconds(10));
                        AtomicAdd(x, 1);
                    });
                    auto t2 = graph.AddFunc(TInc(x), {t1});
                    auto t3 = graph.AddFunc(TInc(x), {t1, t2});
                    auto t4 = graph.AddFunc(TInc(x), {t1, t2});
                    auto t5 = graph.AddFunc(TInc(x), {t3, t4});
                    t5->Wait();
                });
            }
            queue->Stop();
            graph.Stop();
            UNIT_ASSERT_EQUAL_C(AtomicGet(x), 200, "check x");
        }
        Y_UNIT_TEST(TestException) {
            TRTGraph graph(3);
            auto f1 = graph.AddFunc([]() { ythrow yexception() << "exception"; })->GetFuture();
            auto f2 = graph.AddFunc([]() {})->GetFuture();
            f1.Wait();
            f2.Wait();
            UNIT_ASSERT(f1.HasException());
            UNIT_ASSERT(!f2.HasException());
        }

        Y_UNIT_TEST(TestSequentialExceptionAlreadyFail) {
            TRTGraph graph(1);
            auto t1 = graph.AddFunc([]() { ythrow yexception() << "exception"; });
            t1->GetFuture().Wait();
            auto t2 = graph.AddFunc([]() {}, {t1});
            auto f2 = t2->GetFuture();
            f2.Wait();
            UNIT_ASSERT(f2.HasException());
        }
        Y_UNIT_TEST(TestSequentialExceptionBeforeFail) {
            TRTGraph graph(2);
            NThreading::TPromise<void> p = NThreading::NewPromise();
            NThreading::TFuture<void> f = p.GetFuture();
            auto t1 = graph.AddFunc([f]() {f.Wait(); ythrow yexception() << "exception"; });
            auto t2 = graph.AddFunc([]() {}, {t1});
            auto t3 = graph.AddFunc([]() {}, {t2});
            auto t4 = graph.AddFunc([]() {}, {t3});
            p.SetValue();
            t4->Wait();
            auto t5 = graph.AddFunc([]() {}, {t4});
            auto t6 = graph.AddFunc([]() {}, {t5});
            t6->Wait();
            UNIT_ASSERT(t1->HasException());
            UNIT_ASSERT(t2->HasException());
            UNIT_ASSERT(t3->HasException());
            UNIT_ASSERT(t4->HasException());
            UNIT_ASSERT(t5->HasException());
            UNIT_ASSERT(t6->HasException());
        }
        Y_UNIT_TEST(TestParallelExceptionBeforeFail) {
            TRTGraph graph(2);
            NThreading::TPromise<void> p = NThreading::NewPromise();
            NThreading::TFuture<void> f = p.GetFuture();
            auto t1 = graph.AddFunc([f]() {f.Wait(); ythrow yexception() << "exception"; });
            auto t2 = graph.AddFunc([]() {}, {t1});
            auto t3 = graph.AddFunc([]() {}, {t1});
            auto t4 = graph.AddFunc([]() {}, {t1});
            auto t5 = graph.AddFunc([]() {}, {t2, t3});
            auto t6 = graph.AddFunc([]() {}, {t3, t4});
            p.SetValue();
            t5->Wait();
            t6->Wait();
            auto t7 = graph.AddFunc([]() {}, {t5, t6});
            t7->Wait();
            UNIT_ASSERT(t1->HasException());
            UNIT_ASSERT(t2->HasException());
            UNIT_ASSERT(t3->HasException());
            UNIT_ASSERT(t4->HasException());
            UNIT_ASSERT(t5->HasException());
            UNIT_ASSERT(t6->HasException());
            UNIT_ASSERT(t7->HasException());
        }
        Y_UNIT_TEST(TestParallelSeveralExceptionBeforeFail) {
            TRTGraph graph(3);
            NThreading::TPromise<void> p = NThreading::NewPromise();
            auto t1 = graph.AddFunc([]() { ythrow yexception() << "exception"; });
            auto t2 = graph.AddFunc([]() { ythrow yexception() << "exception"; });
            auto t3 = graph.AddFunc([]() {}, {t1, t2});
            auto t4 = graph.AddFunc([]() {}, {t2, t1});
            auto t5 = graph.AddFunc([]() {}, {t2, t3});
            auto t6 = graph.AddFunc([]() {}, {t3, t4});
            t5->Wait();
            t6->Wait();
            auto t7 = graph.AddFunc([]() {}, {t5, t6});
            t7->Wait();
            UNIT_ASSERT(t1->HasException());
            UNIT_ASSERT(t2->HasException());
            UNIT_ASSERT(t3->HasException());
            UNIT_ASSERT(t4->HasException());
            UNIT_ASSERT(t5->HasException());
            UNIT_ASSERT(t6->HasException());
            UNIT_ASSERT(t7->HasException());
        }
        Y_UNIT_TEST(EmptyParents) {
            TRTGraph graph(2);
            TAtomic x(0);
            auto t1 = graph.AddFunc([&x]() {
                Sleep(TDuration::MilliSeconds(10000));
                AtomicAdd(x, 1);
            },
                                    {});
            t1->GetFuture().Wait();
            UNIT_ASSERT_EQUAL(x, 1);
        }
        Y_UNIT_TEST(DuplicateParents) {
            TRTGraph graph(2);
            TAtomic x(0);
            NThreading::TPromise<void> p = NThreading::NewPromise();
            NThreading::TFuture<void> f = p.GetFuture();
            auto t1 = graph.AddFunc([f]() {
                f.Wait();
            });
            auto t2 = graph.AddFunc([f, &x]() {
                f.Wait();
                AtomicAdd(x, 1);
            },
                                    {t1, t1, t1, t1});
            p.SetValue();
            t2->GetFuture().Wait();
            UNIT_ASSERT_EQUAL(x, 1);
        }
    }
}
