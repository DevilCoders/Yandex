#include "watchdog.h"

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/threading/future/future.h>

using namespace NThreading;

Y_UNIT_TEST_SUITE(TimeoutWatchdog) {
    Y_UNIT_TEST(RunCallback) {
        const TString timeoutMessage = "done";
        TPromise<TString> promise = NewPromise<TString>();

        auto callback = [&promise, timeoutMessage] () {
            promise.SetValue(timeoutMessage);
            Cerr << Now() << " callback called" << Endl;
        };
        const TTimeoutWatchDogOptions options = TTimeoutWatchDogOptions(TDuration::Seconds(2));
        TFuture<TString> future = promise.GetFuture();

        Cerr << Now() << " Create watchdog (" << options.Timeout << ")" << Endl;
        TAutoPtr<IWatchDog> watchdog = TTimeoutWatchDogHandle::Create(options, callback);
        UNIT_ASSERT(!future.HasValue());

        TInstant start = Now();

        //There is Sleep(TDuration::Seconds(5)) in watchdog factory
        future.Wait(options.Timeout*3 + TDuration::Seconds(5));
        TInstant finish = Now();
        Cerr << finish << " Wait finished: " << (finish - start) << Endl;

        UNIT_ASSERT_VALUES_EQUAL(future.GetValue(), timeoutMessage);
        UNIT_ASSERT((finish - start) > options.Timeout);
        UNIT_ASSERT((finish - start) < options.Timeout + TDuration::Seconds(5));
    }
}
