#include <kernel/common_server/util/queue.h>

#include "events_rate_calcer.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/datetime/base.h>
#include <util/stream/output.h>


TRTYMtpQueue queue;
const ui32 TestMax = 30000;
const ui32 TestStep = 1000;

class TActor: public IObjectInQueue {
private:
    TAtomic& Counter;
    TEventRate<>& Erc;
    const TInstant StartLoc;

public:
    TActor(TAtomic& counter, TEventRate<>& erc)
        : Counter(counter)
        , Erc(erc)
        , StartLoc(Now())
    {
    }

    virtual void Process(void* /*ThreadSpecificResource*/) {
        TMap<ui32, float> rates;
        rates[5] = 0;
        for (; Counter < TestMax;) {
            Erc.Hit();
            Sleep(TDuration::MilliSeconds(1));
            const ui64 count = AtomicIncrement(Counter);
            if (count && (count % TestStep == 0)) {
                const float referenceRate = count / (Now() - StartLoc).SecondsFloat();

                Erc.Rate(rates);
                const float objectRate = rates[5];
                const double relative = (objectRate - referenceRate) / referenceRate;
                Cerr << Now() << " : reference=" << referenceRate << " calcer=" << objectRate << " rel_diff= " << relative << Endl;
            }
        };
    }
};

Y_UNIT_TEST_SUITE(TRtyEventsRateCalcerSuite) {
    Y_UNIT_TEST(Test1) {
        queue.Start(5);

        TInstant Start = Now();
        TEventRate<> erc;

        TAtomic i = 0;

        Y_VERIFY(queue.AddAndOwn(MakeHolder<TActor>(i, erc)), "FAIL");
        Y_VERIFY(queue.AddAndOwn(MakeHolder<TActor>(i, erc)), "FAIL");
        Y_VERIFY(queue.AddAndOwn(MakeHolder<TActor>(i, erc)), "FAIL");
        Y_VERIFY(queue.AddAndOwn(MakeHolder<TActor>(i, erc)), "FAIL");
        Y_VERIFY(queue.AddAndOwn(MakeHolder<TActor>(i, erc)), "FAIL");
        queue.Stop();

        Cerr << 1.0 * (Now() - Start).MilliSeconds() / TestMax << Endl;
        Cerr << Now() - Start << Endl;
        return;
    }
}
