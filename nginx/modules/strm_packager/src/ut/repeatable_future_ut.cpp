#include <random>

#include <nginx/modules/strm_packager/src/common/repeatable_future.h>

#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/vector.h>
#include <util/generic/queue.h>

using namespace NStrm;

Y_UNIT_TEST_SUITE(TRepeatableFutureTest) {
    std::function<void(const TRepFuture<int>::TDataRef&)> MakeChecker(const TVector<int>& etalon, bool& finished) {
        finished = false;
        auto index = MakeSimpleShared<size_t>(0u);
        return [index, &etalon, &finished](const TRepFuture<int>::TDataRef& data) mutable {
            if (data.Empty()) {
                UNIT_ASSERT_VALUES_EQUAL(etalon.size(), *index);
                UNIT_ASSERT(!finished);
                finished = true;
            } else {
                UNIT_ASSERT_LT(*index, etalon.size());
                UNIT_ASSERT_VALUES_EQUAL(etalon[*index], data.Data());
            }
            ++(*index);
        };
    }

    class TCustomException: public yexception {
    };

    Y_UNIT_TEST(TestValues) {
        const TVector<int> data = {8, 6, 0, 9, 4, 7, 6, 2, 3, 9, 4, 8, 2, 5, 3, 9, 4, 0, 9, 8, 3, 2, 3, 2, 8, 7, 0, 8, 2, 9, 3, 4, 3, 4};
        TDeque<bool> finishedFlags;
        TRepPromise<int> promise = TRepPromise<int>::Make();
        TRepFuture<int> future = promise.GetFuture();

        const auto addChecker = [&future, &finishedFlags, &data]() {
            finishedFlags.push_back(false);
            future.AddCallback(MakeChecker(data, finishedFlags.back()));
        };

        for (int d : data) {
            addChecker();
            promise.PutData(d);
        }

        addChecker();
        promise.Finish();
        addChecker();

        UNIT_ASSERT_VALUES_EQUAL(data.size() + 2, finishedFlags.size());

        for (bool f : finishedFlags) {
            UNIT_ASSERT(f);
        }
    } // Y_UNIT_TEST(TestValues)

    Y_UNIT_TEST(TestException) {
        const TVector<int> data = {1, 2, 3};
        TRepPromise<int> promise = TRepPromise<int>::Make();
        TRepFuture<int> future = promise.GetFuture();

        bool finishedFlag;
        future.AddCallback(MakeChecker(data, finishedFlag));

        promise.PutData(1);
        promise.PutData(2);

        UNIT_ASSERT_EXCEPTION(promise.FinishWithException(std::make_exception_ptr(TCustomException())), TCustomException);
        UNIT_ASSERT_EXCEPTION(future.AddCallback(MakeChecker(data, finishedFlag)), TCustomException);
    } // Y_UNIT_TEST(TestException)

    Y_UNIT_TEST(TestConcat) {
        const TVector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9};

        TRepPromise<int> promise1 = TRepPromise<int>::Make();
        TRepPromise<int> promise2 = TRepPromise<int>::Make();
        TRepPromise<int> promise3 = TRepPromise<int>::Make();
        TRepFuture<int> future = Concat<int, false>({promise1.GetFuture(), promise2.GetFuture(), promise3.GetFuture()});

        bool finishedFlag;
        future.AddCallback(MakeChecker(data, finishedFlag));

        promise1.PutData(1);
        promise1.PutData(2);
        promise1.PutData(3);
        promise1.Finish();

        promise2.PutData(4);
        promise2.PutData(5);
        promise2.PutData(6);
        promise2.Finish();

        promise3.PutData(7);
        promise3.PutData(8);
        promise3.PutData(9);
        promise3.Finish();

        UNIT_ASSERT(finishedFlag);
    } // Y_UNIT_TEST(TestConcat)

    Y_UNIT_TEST(TestZeroCapacity) {
        auto promise = TRepPromise<int>::Make(/*capacity=*/0);

        promise.PutData(0);
        promise.PutData(1);
        promise.PutData(2);

        bool finishedFlag;

        const TVector<int> data = {3, 4, 5};
        promise.GetFuture().AddCallback(MakeChecker(data, finishedFlag));

        promise.PutData(3);
        promise.PutData(4);
        promise.PutData(5);
        promise.Finish();

        UNIT_ASSERT(finishedFlag);
    } // Y_UNIT_TEST(TestZeroCapacity)

    Y_UNIT_TEST(TestRestrictedCapacity) {
        auto promise = TRepPromise<int>::Make(/*capacity=*/3);

        promise.PutData(1);
        promise.PutData(2);
        promise.PutData(3);
        promise.PutData(4);
        promise.PutData(5);
        promise.PutData(6);
        promise.Finish();

        bool finishedFlag;
        const TVector<int> data = {4, 5, 6};
        promise.GetFuture().AddCallback(MakeChecker(data, finishedFlag));

        UNIT_ASSERT(finishedFlag);
    } // Y_UNIT_TEST(TestRestrictedCapacity)

    Y_UNIT_TEST(TestEditCallbacks) {
        bool finishedFlag1;
        bool finishedFlag2;
        bool finishedFlag3;

        auto promise = TRepPromise<int>::Make();

        auto future = promise.GetFuture();
        const TVector<int> data = {1, 2, 3, 4, 5, 6};
        const TVector<int> half = {1, 2, 3};
        const TVector<int> half2 = {1, 2, 3, 4};
        auto callbackToRemove = MakeChecker(half, finishedFlag1);
        auto callbackToRemoveId = future.AddCallback(callbackToRemove);

        UNIT_ASSERT(callbackToRemoveId.Defined());

        future.AddCallback(MakeChecker(data, finishedFlag2));

        auto callbackToRemove2 = MakeChecker(half2, finishedFlag3);
        TMaybe<TRepFutureCallbackHolder<int, false>> callbackHolder;
        callbackHolder.ConstructInPlace(future, future.AddCallback(callbackToRemove2));

        promise.PutData(1);
        promise.PutData(2);
        promise.PutData(3);

        future.RemoveCallback(callbackToRemoveId.GetRef());

        promise.PutData(4);

        callbackHolder.Clear();

        promise.PutData(5);
        promise.PutData(6);

        promise.Finish();

        UNIT_ASSERT(finishedFlag2);

        UNIT_ASSERT(!finishedFlag1);
        // Finish checker explicitly to perform final checks
        callbackToRemove(TRepFuture<int>::TDataRef());
        UNIT_ASSERT(finishedFlag1);

        UNIT_ASSERT(!finishedFlag3);
        // Finish checker explicitly to perform final checks
        callbackToRemove2(TRepFuture<int>::TDataRef());
        UNIT_ASSERT(finishedFlag3);

    } // Y_UNIT_TEST(TestEditCallbacks)

    Y_UNIT_TEST(TestApplySimple) {
        auto promise = TRepPromise<int>::Make();
        auto future = promise.GetFuture().template Apply([](int x) {
            return x * x;
        });

        promise.PutData(1);
        promise.PutData(2);
        promise.PutData(3);
        promise.PutData(4);
        promise.PutData(5);
        promise.Finish();

        const TVector<int> data = {1, 4, 9, 16, 25};
        bool finishedFlag;
        future.AddCallback(MakeChecker(data, finishedFlag));
        UNIT_ASSERT(finishedFlag);
    } // Y_UNIT_TEST(TestApplySimple)

    Y_UNIT_TEST(TestApplyThrows) {
        auto promise = TRepPromise<int>::Make();
        auto future = promise.GetFuture().template Apply([](const int& x) {
            throw TCustomException();
            return x;
        });
        promise.PutData(1);
        promise.Finish();

        bool exceptionArrived = false;
        future.AddCallback([&exceptionArrived](const TRepPromise<int>::TDataRef& dataRef) {
            if (dataRef.Exception()) {
                exceptionArrived = true;
                UNIT_ASSERT_EXCEPTION(dataRef.Data(), TCustomException);
            } else {
                UNIT_FAIL("Unexpected callback call");
            }
        });
        UNIT_ASSERT(exceptionArrived);
    } // Y_UNIT_TEST(TestApplyThrows)

    Y_UNIT_TEST(TestApplyPipedException) {
        auto promise = TRepPromise<int>::Make();
        auto future = promise.GetFuture().template Apply([](const int& x) {
            return x;
        });
        promise.PutData(1);
        promise.FinishWithException(std::make_exception_ptr(TCustomException()));

        bool exceptionArrived = false;
        bool dataArrived = false;
        future.AddCallback([&exceptionArrived, &dataArrived](const TRepPromise<int>::TDataRef& dataRef) {
            if (dataRef.Exception()) {
                UNIT_ASSERT(!exceptionArrived);
                UNIT_ASSERT_EXCEPTION(dataRef.Data(), TCustomException);
                exceptionArrived = true;
            } else if (!dataRef.Empty()) {
                UNIT_ASSERT(!dataArrived);
                UNIT_ASSERT(!exceptionArrived);
                UNIT_ASSERT(dataRef.Data() == 1);
                dataArrived = true;
            } else {
                UNIT_FAIL("Unexpected callback call");
            }
        });
        UNIT_ASSERT(exceptionArrived);
        UNIT_ASSERT(dataArrived);
    } // Y_UNIT_TEST(TestApplyPipedException)

    Y_UNIT_TEST(TestApplyPreservesCapacity) {
        auto promise = TRepPromise<int>::Make(/*capacity=*/10);
        auto future = promise.GetFuture().template Apply([](const int& x) {
            return x;
        });
        UNIT_ASSERT_VALUES_EQUAL(future.GetCapacity(), 10);
    } // Y_UNIT_TEST(TestApplyPreservesCapacity)

    Y_UNIT_TEST(TestReceiverSetThenGet) {
        const TVector<int> data{4, 1, 4, 5, 6, 7, 8, 9};
        auto promise = TRepPromise<int, true>::Make();

        for (int i : data) {
            promise.PutData(i);
        }
        promise.Finish();
        TRepFutureReceiver<int> receiver(promise.GetFuture());

        for (int i : data) {
            auto future = receiver.GetFuture();
            UNIT_ASSERT(future.HasValue());
            UNIT_ASSERT_VALUES_EQUAL(future.ExtractValue(), i);
        }

        UNIT_ASSERT(receiver.GetFuture().GetValue().Empty());
        UNIT_ASSERT(receiver.GetFuture().GetValue().Empty());
    } // Y_UNIT_TEST(TestReceiverSetThenGet)

    Y_UNIT_TEST(TestReceiverGetThenSet) {
        const TVector<int> data{4, 1, 4, 5, 6, 7, 8, 9};
        TVector<NThreading::TFuture<TMaybe<int>>> futures;

        auto promise = TRepPromise<int, true>::Make();
        TRepFutureReceiver<int> receiver(promise.GetFuture());

        // Add some more futures in the end to ensure that they will hold empty TMaybe
        for (size_t i = 0; i < data.size() + 3; ++i) {
            futures.push_back(receiver.GetFuture());
        }

        for (const auto& future : futures) {
            UNIT_ASSERT(!future.HasValue());
            UNIT_ASSERT(!future.HasException());
        }

        for (int i : data) {
            promise.PutData(i);
        }
        promise.Finish();

        for (size_t i = 0; i < futures.size(); ++i) {
            auto& future = futures[i];
            if (i < data.size()) {
                UNIT_ASSERT(future.HasValue());
                UNIT_ASSERT_VALUES_EQUAL(data[i], future.ExtractValue());
            } else {
                UNIT_ASSERT(future.ExtractValue().Empty());
            }
        }
    } // Y_UNIT_TEST(TestReceiverGetThenSet)

    Y_UNIT_TEST(TestReceiverMixed) {
        auto promise = TAtomicRepPromise<int>::Make();
        TRepFutureReceiver<int> receiver(promise.GetFuture());
        auto future = receiver.GetFuture();
        for (int i = 1; i <= 100; ++i) {
            promise.PutData(i);
            // Read every 10 iterations
            if (i % 10 == 0) {
                TVector<int> available;
                UNIT_ASSERT(!future.HasException());
                while (future.HasValue()) {
                    auto value = future.ExtractValue();
                    UNIT_ASSERT(value.Defined());
                    available.push_back(value.GetRef());
                    future = receiver.GetFuture();
                }
                // Check that we have read last 10 numbers in right order
                UNIT_ASSERT_VALUES_EQUAL(available.size(), 10);
                for (int j = 0; j < 10; ++j) {
                    UNIT_ASSERT_VALUES_EQUAL(available[j], i - 9 + j);
                }
            }
        }
        promise.Finish();
        UNIT_ASSERT(receiver.GetFuture().GetValue().Empty());
    } // Y_UNIT_TEST(TestReceiverMixed)

    Y_UNIT_TEST(TestReceiverGetThenFinish) {
        auto promise = TAtomicRepPromise<int>::Make();
        TRepFutureReceiver<int> receiver(promise.GetFuture());

        auto future1 = receiver.GetFuture();
        auto future2 = receiver.GetFuture();
        auto future3 = receiver.GetFuture();
        auto future4 = receiver.GetFuture();

        promise.PutData(1);
        promise.Finish();

        UNIT_ASSERT_VALUES_EQUAL(future1.GetValue(), 1);
        UNIT_ASSERT_VALUES_EQUAL(future2.GetValue(), TMaybe<int>());
        UNIT_ASSERT_VALUES_EQUAL(future3.GetValue(), TMaybe<int>());
        UNIT_ASSERT_VALUES_EQUAL(future4.GetValue(), TMaybe<int>());
    } // Y_UNIT_TEST(TestReceiverGetThenFinish)

    Y_UNIT_TEST(TestReceiverFinishThenGet) {
        auto promise = TAtomicRepPromise<int>::Make();
        TRepFutureReceiver<int> receiver(promise.GetFuture());

        promise.PutData(1);
        promise.Finish();

        auto future1 = receiver.GetFuture();
        auto future2 = receiver.GetFuture();
        auto future3 = receiver.GetFuture();
        auto future4 = receiver.GetFuture();

        UNIT_ASSERT_VALUES_EQUAL(future1.GetValue(), 1);
        UNIT_ASSERT_VALUES_EQUAL(future2.GetValue(), TMaybe<int>());
        UNIT_ASSERT_VALUES_EQUAL(future3.GetValue(), TMaybe<int>());
        UNIT_ASSERT_VALUES_EQUAL(future4.GetValue(), TMaybe<int>());
    } // Y_UNIT_TEST(TestReceiverFinishThenGet)

    Y_UNIT_TEST(TestReceiverGetThenException) {
        auto promise = TAtomicRepPromise<int>::Make();
        TRepFutureReceiver<int> receiver(promise.GetFuture());

        auto future1 = receiver.GetFuture();
        auto future2 = receiver.GetFuture();
        auto future3 = receiver.GetFuture();
        auto future4 = receiver.GetFuture();

        promise.PutData(1);
        promise.FinishWithException(std::make_exception_ptr(TCustomException()));

        UNIT_ASSERT_VALUES_EQUAL(future1.GetValue(), 1);
        UNIT_ASSERT_EXCEPTION(future2.GetValue(), TCustomException);
        UNIT_ASSERT_EXCEPTION(future3.GetValue(), TCustomException);
        UNIT_ASSERT_EXCEPTION(future4.GetValue(), TCustomException);
    } // Y_UNIT_TEST(TestReceiverGetThenException)

    Y_UNIT_TEST(TestReceiverExceptionThenGet) {
        auto promise = TAtomicRepPromise<int>::Make();
        TRepFutureReceiver<int> receiver(promise.GetFuture());

        promise.PutData(1);
        promise.FinishWithException(std::make_exception_ptr(TCustomException()));

        auto future1 = receiver.GetFuture();
        auto future2 = receiver.GetFuture();
        auto future3 = receiver.GetFuture();
        auto future4 = receiver.GetFuture();

        UNIT_ASSERT_VALUES_EQUAL(future1.GetValue(), 1);
        UNIT_ASSERT_EXCEPTION(future2.GetValue(), TCustomException);
        UNIT_ASSERT_EXCEPTION(future3.GetValue(), TCustomException);
        UNIT_ASSERT_EXCEPTION(future4.GetValue(), TCustomException);
    }

    Y_UNIT_TEST(TestQueueReceiverData) {
        auto promise = TAtomicRepPromise<int>::Make();

        promise.PutData(-1);
        promise.PutData(0);

        TRepFutureQueueReceiver receiver(promise.GetFuture());

        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), -1);
        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), 0);
        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), TMaybe<int>());
        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), TMaybe<int>());
        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), TMaybe<int>());
        promise.PutData(1);
        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), 1);
        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), TMaybe<int>());
        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), TMaybe<int>());
        promise.PutData(2);
        promise.PutData(3);
        promise.PutData(4);
        promise.PutData(5);
        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), 2);
        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), 3);
        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), 4);
        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), 5);
        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), TMaybe<int>());
        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), TMaybe<int>());
        promise.Finish();
        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), TMaybe<int>());
        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), TMaybe<int>());
    }

    Y_UNIT_TEST(TestQueueReceiverException) {
        auto promise = TAtomicRepPromise<int>::Make();

        promise.PutData(-1);
        promise.PutData(0);

        TRepFutureQueueReceiver receiver(promise.GetFuture());

        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), -1);
        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), 0);
        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), TMaybe<int>());
        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), TMaybe<int>());
        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), TMaybe<int>());
        promise.PutData(1);
        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), 1);
        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), TMaybe<int>());
        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), TMaybe<int>());
        promise.PutData(2);
        promise.PutData(3);
        promise.PutData(4);
        promise.PutData(5);
        promise.FinishWithException(std::make_exception_ptr(TCustomException()));

        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), 2);
        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), 3);
        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), 4);
        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), 5);

        UNIT_ASSERT_EXCEPTION(receiver.Pop(), TCustomException);
        UNIT_ASSERT_EXCEPTION(receiver.Pop(), TCustomException);
        UNIT_ASSERT_EXCEPTION(receiver.Pop(), TCustomException);
        UNIT_ASSERT_EXCEPTION(receiver.Pop(), TCustomException);
    }

    Y_UNIT_TEST(TestQueueReceiverUnsubscribe) {
        auto promise = TAtomicRepPromise<int>::Make();

        promise.PutData(-1);
        promise.PutData(0);

        TRepFutureQueueReceiver receiver(promise.GetFuture());

        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), -1);
        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), 0);
        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), TMaybe<int>());
        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), TMaybe<int>());
        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), TMaybe<int>());
        promise.PutData(1);
        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), 1);
        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), TMaybe<int>());
        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), TMaybe<int>());
        promise.PutData(2);
        promise.PutData(3);

        receiver.Unsubscribe();

        promise.PutData(4);
        promise.PutData(5);
        promise.FinishWithException(std::make_exception_ptr(TCustomException()));

        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), 2);
        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), 3);

        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), TMaybe<int>());
        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), TMaybe<int>());
        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), TMaybe<int>());
        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), TMaybe<int>());
        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), TMaybe<int>());
        UNIT_ASSERT_VALUES_EQUAL(receiver.Pop(), TMaybe<int>());
    }

} // Y_UNIT_TEST_SUITE(TRepeatableFutureTest)
