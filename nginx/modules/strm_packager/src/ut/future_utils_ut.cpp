#include <random>

#include <nginx/modules/strm_packager/src/common/future_utils.h>

#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/vector.h>

using namespace NStrm::NPackager;

Y_UNIT_TEST_SUITE(PackagerWaitExceptionOrAllTest) {
    class TCustomException: public yexception {
    };

    Y_UNIT_TEST(TestValue) {
        TVector<NThreading::TFuture<int>> futures;

        futures.push_back(NThreading::MakeFuture(1));
        futures.push_back(NThreading::MakeFuture(2));
        futures.push_back(NThreading::MakeFuture(3));

        NThreading::TFuture<void> future = PackagerWaitExceptionOrAll(futures);

        UNIT_ASSERT(future.HasValue());
    } // Y_UNIT_TEST(TestValue)

    Y_UNIT_TEST(TestException) {
        TVector<NThreading::TFuture<int>> futures;

        futures.push_back(NThreading::MakeFuture(1));
        futures.push_back(NThreading::MakeErrorFuture<int>(std::make_exception_ptr(TCustomException())));
        futures.push_back(NThreading::MakeFuture(3));

        NThreading::TFuture<void> future = PackagerWaitExceptionOrAll(futures);

        UNIT_ASSERT(future.HasException());
    } // Y_UNIT_TEST(TestException)

    Y_UNIT_TEST(TestExceptionCallback) {
        TVector<NThreading::TPromise<int>> promises;
        promises.push_back(NThreading::NewPromise<int>());
        promises.push_back(NThreading::NewPromise<int>());
        promises.push_back(NThreading::NewPromise<int>());

        NThreading::TFuture<void> future = PackagerWaitExceptionOrAll(TVector<NThreading::TFuture<int>>(promises.begin(), promises.end()));

        future.Subscribe([](const NThreading::TFuture<void>&) {
            throw TCustomException();
        });

        promises[0].SetValue(1);
        promises[1].SetValue(2);

        UNIT_ASSERT_EXCEPTION(promises[2].SetValue(3), TCustomException);
    } // Y_UNIT_TEST(TestExceptionCallback)
} // Y_UNIT_TEST_SUITE(PackagerWaitExceptionOrAllTest)
