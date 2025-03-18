#include "histogram.h"
#include <library/cpp/logger/priority.h>
#include <library/cpp/logger/global/global.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/thread/pool.h>
#include <util/random/random.h>
#include "fix_interval.h"
#include "time_slide.h"
#include <rtline/util/events_rate_calcer.h>
#include <kernel/daemon/common/time_guard.h>
#include <util/string/builder.h>

void InitLog(const ELogPriority logPriority) {
    DoInitGlobalLog("cout", logPriority, false, false);
}

Y_UNIT_TEST_SUITE(Histograms) {
    template <class THist>
    class THistAction: public IObjectInQueue {
    private:
        THist& Hist;
        const double Value;

    public:
        THistAction(THist& hist, const double value)
            : Hist(hist)
            , Value(value)
        {
        }

        void Process(void* /*threadSpecificResource*/) override {
            NanoSleep(RandomNumber<double>() * 100);
            Hist.Add(Value);
        }
    };

    Y_UNIT_TEST(Simple) {
        InitLog(TLOG_DEBUG);
        TFixIntervalHistogram hist(-10, 100, 11);
        TThreadPool queue;
        queue.Start(32);
        for (ui32 i = 0; i < 100000; ++i) {
            queue.SafeAddAndOwn(MakeHolder<THistAction<TFixIntervalHistogram>>(hist, ::RandomNumber<double>() * 130 - 20));
        }
        queue.Stop();
        TVector<ui32> distr = hist.Clone();
        ui64 sum = 0;
        for (ui32 i = 0; i < distr.size(); ++i) {
            sum += distr[i];
        }
        UNIT_ASSERT_VALUES_EQUAL(sum, 100000);
        for (ui32 i = 0; i < distr.size(); ++i) {
            const double p = 1.0 * distr[i] / sum * distr.size() - 1;
            INFO_LOG << hist.GetIntervalName(i) << ": " << distr[i] << "(" << p << ")" << Endl;
            UNIT_ASSERT_C(p < 0.05, (TStringBuilder() << "p = " << p));
        }
    }

    template <class TCounter>
    class TCountActor: public IObjectInQueue {
    private:
        TCounter& Counter;
        const TDuration SleepDuration;
        const ui32 CountMessages;

    public:
        TCountActor(TCounter& counter, const ui32 sleepms = 0, const ui32 countMessages = 65000000)
            : Counter(counter)
            , SleepDuration(TDuration::MilliSeconds(sleepms))
            , CountMessages(countMessages)
        {
        }

        void Process(void* /*threadSpecificResource*/) override {
            for (ui32 i = 0; i < CountMessages; ++i) {
                if (SleepDuration != TDuration::Zero()) {
                    Sleep(SleepDuration);
                }
                Counter.Hit();
            }
        }
    };

    Y_UNIT_TEST(SlideCounterMTLong) {
        TThreadPool queue;
        {
            queue.Start(4);
            TTimeSlidedCounter counter(TDuration::Seconds(10), 60);
            TDuration dur;
            {
                TTimeGuard tg(&dur, "slider");
                for (ui32 i = 0; i < 4; ++i) {
                    queue.SafeAddAndOwn(MakeHolder<TCountActor<TTimeSlidedCounter>>(counter, 10, 2000));
                }
                queue.Stop();
            }
            const double rps = 1000 * 2000.0 * 4 / (dur.MilliSeconds());
            const double counterVal = counter.Get() / 10.0;
            UNIT_ASSERT_C(Abs(counterVal / rps - 1) < 1e-1, (TStringBuilder() << counterVal << " / " << rps));
        }
    }

    Y_UNIT_TEST(SlideCounterMT) {
        TThreadPool queue;
        {
            queue.Start(4);
            TTimeSlidedCounter counter(TDuration::Minutes(1), 60);
            TTimeGuard tg("slider");
            for (ui32 i = 0; i < 4; ++i) {
                queue.SafeAddAndOwn(MakeHolder<TCountActor<TTimeSlidedCounter>>(counter));
            }
            queue.Stop();
            const ui32 counterVal = counter.Get();
            UNIT_ASSERT_VALUES_EQUAL(counterVal, 260000000);
            TTimeSlidedCounter counterCopy;
            UNIT_ASSERT(counterCopy.Deserialize(counter.Serialize()));
            UNIT_ASSERT_EQUAL(counterCopy, counter);
        }

        {
            queue.Start(4);
            TEventRate<> counter;
            TTimeGuard tg("events");
            for (ui32 i = 0; i < 4; ++i) {
                queue.SafeAddAndOwn(MakeHolder<TCountActor<TEventRate<>>>(counter));
            }
            queue.Stop();
            INFO_LOG << 1.0 * counter.Get(TDuration::Minutes(10)) << Endl;
        }
    }

    Y_UNIT_TEST(SlideCounterSimple) {
        {
            TTimeSlidedCounter counter(TDuration::Minutes(1), 60);
            for (ui32 i = 0; i < 5000; ++i) {
                Sleep(TDuration::MilliSeconds(3));
                counter.Hit();
            }
            INFO_LOG << 1.0 * counter.Get() / 60 << Endl;
        }

        {
            TEventRate<> counter;
            for (ui32 i = 0; i < 5000; ++i) {
                Sleep(TDuration::MilliSeconds(3));
                counter.Hit();
            }
            INFO_LOG << 1.0 * counter.Get(TDuration::Minutes(10)) << Endl;
        }
    }

    Y_UNIT_TEST(SlideSimple) {
        InitLog(TLOG_DEBUG);
        TTimeSlidedHistogram hist(100, 20, -10, 100, 11);
        TThreadPool queue;
        queue.Start(32);
        for (ui32 i = 0; i < 1000000; ++i) {
            UNIT_ASSERT(queue.AddAndOwn(MakeHolder<THistAction<TTimeSlidedHistogram>>(hist, ::RandomNumber<double>() * 130 - 20)));
        }
        queue.Stop();
        TVector<ui32> distr = hist.Clone();
        ui64 sum = 0;
        for (ui32 i = 0; i < distr.size(); ++i) {
            sum += distr[i];
        }
        UNIT_ASSERT_VALUES_EQUAL(sum, 1000000);
        for (ui32 i = 0; i < distr.size(); ++i) {
            const double p = 1.0 * distr[i] / sum * distr.size() - 1;
            INFO_LOG << hist.GetIntervalName(i) << ": " << distr[i] << "(" << p << ")" << Endl;
            UNIT_ASSERT_C(p < 0.05, (TStringBuilder() << "p = " << p));
        }
    }

    Y_UNIT_TEST(SlideSimpleRewrite) {
        InitLog(TLOG_DEBUG);
        TTimeSlidedHistogram hist(100, 1, -10, 100, 11);
        TThreadPool queue;
        queue.Start(32);
        for (ui32 i = 0; i < 5000000; ++i) {
            UNIT_ASSERT(queue.AddAndOwn(MakeHolder<THistAction<TTimeSlidedHistogram>>(hist, ::RandomNumber<double>() * 130 - 20)));
        }
        queue.Stop();
        TVector<ui32> distr = hist.Clone();
        ui64 sum = 0;
        for (ui32 i = 0; i < distr.size(); ++i) {
            sum += distr[i];
        }
        UNIT_ASSERT_VALUES_UNEQUAL(sum, 5000000);
        for (ui32 i = 0; i < distr.size(); ++i) {
            const double p = 1.0 * distr[i] / sum * distr.size() - 1;
            INFO_LOG << hist.GetIntervalName(i) << ": " << distr[i] << "(" << p << ")" << Endl;
            UNIT_ASSERT_C(p < 0.05, (TStringBuilder() << "p = " << p));
        }
    }

    Y_UNIT_TEST(SlideSimpleWait) {
        InitLog(TLOG_DEBUG);
        TTimeSlidedHistogram hist(100, 2, -10, 100, 11);
        TThreadPool queue;
        queue.Start(32);
        for (ui32 i = 0; i < 10000; ++i) {
            UNIT_ASSERT(queue.AddAndOwn(MakeHolder<THistAction<TTimeSlidedHistogram>>(hist, ::RandomNumber<double>() * 130 - 20)));
        }
        queue.Stop();
        sleep(3);
        TVector<ui32> distr = hist.Clone();
        ui64 sum = 0;
        for (ui32 i = 0; i < distr.size(); ++i) {
            sum += distr[i];
        }
        UNIT_ASSERT_VALUES_EQUAL(sum, 0);
    }
}
