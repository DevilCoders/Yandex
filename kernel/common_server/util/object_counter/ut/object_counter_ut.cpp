#include <thread>
#include <chrono>

#include <kernel/common_server/util/object_counter/object_counter.h>
#include <kernel/common_server/util/object_counter/stats_sender.h>
#include <kernel/common_server/library/logging/events.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/common/network.h>
#include <library/cpp/json/json_reader.h>

#include <util/system/thread.h>

class Dummy: public NCSUtil::TObjectCounter<Dummy> {
};

class DummyTwo: public NCSUtil::TObjectCounter<DummyTwo> {
};

class Simple {
};

namespace NFirst {
    namespace NSecond {
        template <class T>
        struct Complex : Simple {
            void f();
            int a;
        };
    }
}

bool WaitForExpected(const NMonitoring::TLabels& labels, ui64 expected) {
    const TDuration maxWaitTime = TDuration::Seconds(10);
    const TDuration waitTimeQuant = TDuration::MilliSeconds(500);
    TDuration totalWaitTime;
    while (totalWaitTime <= maxWaitTime) {
        if (TCSSignals::GetValueGauge(labels) == expected) {
            return true;
        }
        totalWaitTime += waitTimeQuant;
        Sleep(waitTimeQuant);
    }
    return false;
}

template <class T>
void CheckDemangle(const THashSet<TString>& expected) {
    CHECK_WITH_LOG(expected.contains(CppDemangle(typeid(T).name()))) << CppDemangle(typeid(T).name());
}

Y_UNIT_TEST_SUITE(Counter) {
    Y_UNIT_TEST(Simple) {
        TMonitoringConfig config;
        auto holder = NTesting::GetFreePort();
        config.MutableHttpOptions().SetPort((ui16)holder);
        TCSSignals::RegisterMonitoring(config);
        NMonitoring::TLabels labels;
        labels.Add("object_type", CppDemangle(typeid(Dummy).name()));
        labels.Add("sensor", "objects_count");

        {
            const auto sendPeriod = NCSUtil::TStatsSender::kSendPeriod;
            TVector<THolder<IThreadFactory::IThread>> workers;
            const ui64 num = 100;
            for (size_t i = 0; i < num; ++i) {
                workers.emplace_back(SystemThreadFactory()->Run([sendPeriod]() {
                    Dummy d;
                    Sleep(2 * sendPeriod);
                }));
            }
            CHECK_WITH_LOG(WaitForExpected(labels, 100)) << TCSSignals::GetValueGauge(labels);
            for (auto&& w : workers) {
                w->Join();
            }
            CHECK_WITH_LOG(WaitForExpected(labels, 0)) << TCSSignals::GetValueGauge(labels);
        }
        {
            Dummy d1;
            Dummy d2;
            CHECK_WITH_LOG(WaitForExpected(labels, 2)) << TCSSignals::GetValueGauge(labels);
        }
        {
            Dummy d1;
            Dummy d2 = d1;
            Dummy d3(d2);
            CHECK_WITH_LOG(WaitForExpected(labels, 3)) << TCSSignals::GetValueGauge(labels);

        }
        {
            Dummy d1;
            Dummy d2 = std::move(d1);
            Dummy d3(std::move(d2));
            CHECK_WITH_LOG(WaitForExpected(labels, 3)) << TCSSignals::GetValueGauge(labels);
        }
        CHECK_WITH_LOG(WaitForExpected(labels, 0)) << TCSSignals::GetValueGauge(labels);
        {
            Dummy d1;
            DummyTwo d2;
            NMonitoring::TLabels labels;
            labels.Add("object_type", CppDemangle(typeid(Dummy).name()));
            labels.Add("sensor", "objects_count");
            CHECK_WITH_LOG(WaitForExpected(labels, 1)) << TCSSignals::GetValueGauge(labels);
            NMonitoring::TLabels labelsTwo;
            labelsTwo.Add("object_type", CppDemangle(typeid(DummyTwo).name()));
            labelsTwo.Add("sensor", "objects_count");
            CHECK_WITH_LOG(WaitForExpected(labelsTwo, 1)) << TCSSignals::GetValueGauge(labelsTwo);
        }
        CHECK_WITH_LOG(WaitForExpected(labels, 0)) << TCSSignals::GetValueGauge(labels);

        TCSSignals::UnregisterMonitoring();
    }

    Y_UNIT_TEST(Demangle) {
        CheckDemangle<Simple>({"Simple", "class Simple"});
        CheckDemangle<NFirst::NSecond::Complex<Simple>>({"NFirst::NSecond::Complex<Simple>", "struct NFirst::NSecond::Complex<class Simple>"});
    }
}
