#include <util/thread/factory.h>
#include <functional>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/unistat/unistat.h>
#include <library/cpp/unistat/raii.h>

enum class EUnistatTest {
    Shots,
    Owl,
    Rabbit
};

static TString ToString(const EUnistatTest v) {
    switch (v) {
        case EUnistatTest::Shots:
            return "Shots";
        case EUnistatTest::Owl:
            return "Owl";
        case EUnistatTest::Rabbit:
            return "Rabbit";
    };
}

class TTestIniter {
public:
    void Init(TUnistat& t) const {
        static TVector<double> intervals = {0.0, 10.0, 100.0, 1000.0};
        t.DrillHistogramHole("Shots", "Shots description", "ahhh", NUnistat::TPriority(10), intervals);
        t.DrillFloatHole("Owl", "Owl description", "ahhh", NUnistat::TPriority(10));
        t.DrillFloatHole("Rabbit", "Rabbit description", "ahhh", NUnistat::TPriority(10));
    }
    void AddTags(TUnistat& t) const {
        t.AddGlobalTag("forest", "calm");
        t.DrillFloatHole("Owl", "Owl description", "ahhh", NUnistat::TPriority(10))->AddTag("wise", "yes");
    }
};

void KayakKayak(std::function<void()> exec, size_t numthreads) {
    TVector<THolder<IThreadFactory::IThread>> pool;
    for (size_t i = 0; i < numthreads; ++i) {
        THolder<IThreadFactory::IThread> thread(SystemThreadFactory()->Run(exec).Release());
        pool.emplace_back(std::move(thread));
    };

    for (size_t i = 0; i < numthreads; ++i) {
        pool[i]->Join();
    }
}

NJson::TJsonValue JsonFromString(const TString& string) {
    TStringInput output(string);
    NJson::TJsonValue value;
    NJson::ReadJsonTree(&output, &value);
    return value;
}

Y_UNIT_TEST_SUITE(TUnistatTests) {
    Y_UNIT_TEST(GetSignalDescriptions) {
        TUnistat::Instance().Reset();
        TTestIniter initer;
        initer.Init(TUnistat::Instance());
        NJson::TJsonValue descriptions = JsonFromString(TUnistat::Instance().GetSignalDescriptions());
        UNIT_ASSERT_VALUES_EQUAL(descriptions["Shots"], "Shots description");
        UNIT_ASSERT_VALUES_EQUAL(descriptions["Owl"], "Owl description");
        UNIT_ASSERT_VALUES_EQUAL(descriptions["Rabbit"], "Rabbit description");
    }

    Y_UNIT_TEST(GetHolenames) {
        TUnistat::Instance().Reset();
        TTestIniter initer;
        initer.Init(TUnistat::Instance());
        TVector<TString> holenames = TUnistat::Instance().GetHolenames();
        Sort(holenames.begin(), holenames.end());
        const TVector<TString> expectedHolenames = {
            "Owl"
            , "Rabbit"
            , "Shots"
        };
        UNIT_ASSERT_VALUES_EQUAL(holenames, expectedHolenames);
    }

    Y_UNIT_TEST(EraseHole) {
        TUnistat::Instance().Reset();
        TTestIniter initer;
        initer.Init(TUnistat::Instance());

        UNIT_ASSERT(!TUnistat::Instance().EraseHole("RandomName"));
        UNIT_ASSERT_VALUES_EQUAL(TUnistat::Instance().GetHolenames().size(), 3);

        UNIT_ASSERT(TUnistat::Instance().EraseHole("Rabbit"));
        TVector<TString> holenames = TUnistat::Instance().GetHolenames();
        Sort(holenames.begin(), holenames.end());
        const TVector<TString> expectedHolenames = {
            "Owl"
            , "Shots"
        };
        UNIT_ASSERT_VALUES_EQUAL(holenames, expectedHolenames);
    }

    Y_UNIT_TEST(FloatHolePush) {
        TUnistat::Instance().Reset();
        TTestIniter initer;
        initer.Init(TUnistat::Instance());
        std::function<void()> exec = []() {
            TUnistat& t = TUnistat::Instance();
            t.PushSignalUnsafe("Rabbit", 0.1);
            t.PushSignalUnsafe("Owl", 10);
        };
        KayakKayak(exec, 100);
        NJson::TJsonValue value = JsonFromString(TUnistat::Instance().CreateInfoDump(10));
        UNIT_ASSERT_VALUES_EQUAL(value["Rabbit"]["Value"], 10);
        UNIT_ASSERT_VALUES_EQUAL(value["Owl"]["Value"], 1000);
    }

    Y_UNIT_TEST(FloatHolePushTagged) {
        TUnistat::Instance().Reset();
        TTestIniter initer;
        initer.Init(TUnistat::Instance());
        initer.AddTags(TUnistat::Instance());
        std::function<void()> exec = []() {
            TUnistat& t = TUnistat::Instance();
            t.PushSignalUnsafe("Rabbit", 0.1);
            t.PushSignalUnsafe("Owl", 10);
        };
        KayakKayak(exec, 100);
        NJson::TJsonValue value = JsonFromString(TUnistat::Instance().CreateInfoDump(10));
        UNIT_ASSERT_VALUES_EQUAL(value["Rabbit"]["Value"], 10);
        UNIT_ASSERT_VALUES_EQUAL(value["Owl"]["Value"], 1000);
        UNIT_ASSERT_VALUES_EQUAL(value["Rabbit"]["Tags"], "forest=calm;");
        UNIT_ASSERT_VALUES_EQUAL(value["Owl"]["Tags"], "forest=calm;wise=yes;");
    }

    Y_UNIT_TEST(HistogramHolePush) {
        TUnistat::Instance().Reset();
        TTestIniter initer;
        initer.Init(TUnistat::Instance());
        std::function<void()> exec = []() {
            TUnistat& t = TUnistat::Instance();
            for (int i = -5; i < 10; i++) {
                t.PushSignalUnsafe("Shots", i);
                t.PushSignalUnsafe("Shots", i + 0.1);
                t.PushSignalUnsafe("Shots", i * 10 + 0.1);
                t.PushSignalUnsafe("Shots", i * 100 + 0.1);
                t.PushSignalUnsafe("Shots", i * 1000 + 0.1);
            }
        };
        KayakKayak(exec, 100);
        NJson::TJsonValue value = JsonFromString(TUnistat::Instance().CreateInfoDump(10));
        const auto& stats = value["Shots"]["Value"];
        UNIT_ASSERT_VALUES_EQUAL(stats[0][1], 2300);
        UNIT_ASSERT_VALUES_EQUAL(stats[1][1], 900);
        UNIT_ASSERT_VALUES_EQUAL(stats[2][1], 900);
        UNIT_ASSERT_VALUES_EQUAL(stats[3][1], 900);
    }

    Y_UNIT_TEST(HistogramHoleNoNegativeIntegers) {
        using namespace NUnistat;
        TVector<double> intervals{ 0, 10, 20 };
        THistogramHole hist("test", "", "ahhh", EAggregationType::HostHistogram, 10, intervals, false);
        hist.SetWeight(0, Max<ui32>());

        NJsonWriter::TBuf json;
        json.BeginObject();
        hist.PrintInfo(json);
        json.EndObject();
        auto parsedJson = JsonFromString(json.Str());
        UNIT_ASSERT_VALUES_EQUAL(parsedJson["test"]["Value"][0][1], Max<ui32>());
    }

    Y_UNIT_TEST(TestPushUnsafeOverloads) {
        TUnistat t;
        TTestIniter initer;

        {
            t.Reset();
            initer.Init(t);
            t.PushSignalUnsafe(EUnistatTest::Rabbit, 1);
            t.PushSignalUnsafe(EUnistatTest::Owl, 2);
            const auto value = JsonFromString(t.CreateInfoDump(10));
            UNIT_ASSERT_VALUES_EQUAL(value["Rabbit"]["Value"], 1);
            UNIT_ASSERT_VALUES_EQUAL(value["Owl"]["Value"], 2);
        }

        {
            t.Reset();
            initer.Init(t);
            const TString rabbit = "Rabbit";
            const TString owl = "Owl";
            t.PushSignalUnsafe(rabbit.data(), 1);
            t.PushSignalUnsafe(owl.data(), 2);
            const auto value = JsonFromString(t.CreateInfoDump(10));
            UNIT_ASSERT_VALUES_EQUAL(value["Rabbit"]["Value"], 1);
            UNIT_ASSERT_VALUES_EQUAL(value["Owl"]["Value"], 2);
        }

        {
            t.Reset();
            initer.Init(t);
            char rabbit[] = "Rabbit";
            char owl[] = "Owl";
            t.PushSignalUnsafe(rabbit, 1);
            t.PushSignalUnsafe(owl, 2);
            const auto value = JsonFromString(t.CreateInfoDump(10));
            UNIT_ASSERT_VALUES_EQUAL(value["Rabbit"]["Value"], 1);
            UNIT_ASSERT_VALUES_EQUAL(value["Owl"]["Value"], 2);
        }

        {
            t.Reset();
            initer.Init(t);
            const char rabbit[] = "Rabbit";
            const char owl[] = "Owl";
            t.PushSignalUnsafe(rabbit, 1);
            t.PushSignalUnsafe(owl, 2);
            const auto value = JsonFromString(t.CreateInfoDump(10));
            UNIT_ASSERT_VALUES_EQUAL(value["Rabbit"]["Value"], 1);
            UNIT_ASSERT_VALUES_EQUAL(value["Owl"]["Value"], 2);
        }

        {
            t.Reset();
            initer.Init(t);
            const TStringBuf rabbit = "Rabbit";
            const TStringBuf owl = "Owl";
            t.PushSignalUnsafe(rabbit, 1);
            t.PushSignalUnsafe(owl, 2);
            const auto value = JsonFromString(t.CreateInfoDump(10));
            UNIT_ASSERT_VALUES_EQUAL(value["Rabbit"]["Value"], 1);
            UNIT_ASSERT_VALUES_EQUAL(value["Owl"]["Value"], 2);
        }
    }

    Y_UNIT_TEST(TestRAIITimer) {
        TUnistat t;
        TTestIniter initer;

        {
            {
                t.Reset();
                initer.Init(t);
                TUnistatTimer timer{ t, EUnistatTest::Rabbit };
                Sleep(TDuration::MilliSeconds(10));
            }
            const auto value = JsonFromString(t.CreateInfoDump(10));
            UNIT_ASSERT(value["Rabbit"]["Value"].GetDoubleRobust() >= 10.);
        }
        {
            {
                t.Reset();
                initer.Init(t);
                TUnistatTimer timer{ t, EUnistatTest::Rabbit };
                Sleep(TDuration::MilliSeconds(10));
                timer.Dismiss();
            }
            const auto value = JsonFromString(t.CreateInfoDump(10));
            UNIT_ASSERT_VALUES_EQUAL(value["Rabbit"]["Value"].GetDoubleRobust(), 0.0);
        }
        {
            {
                t.Reset();
                initer.Init(t);
                TUnistatTimer timer{ t, EUnistatTest::Rabbit };
                Sleep(TDuration::MilliSeconds(10));
                timer.Dismiss();
                timer.Accept();
            }
            const auto value = JsonFromString(t.CreateInfoDump(10));
            UNIT_ASSERT(value["Rabbit"]["Value"].GetDoubleRobust() >= 10.);
        }
        {
            {
                t.Reset();
                initer.Init(t);
                Y_UNISTAT_TIMER(t, EUnistatTest::Rabbit)
                    Sleep(TDuration::MilliSeconds(10));
            }
            const auto value = JsonFromString(t.CreateInfoDump(10));
            UNIT_ASSERT(value["Rabbit"]["Value"].GetDoubleRobust() >= 10.);
        }
    }

    Y_UNIT_TEST(TestRAIIExceptionCounter) {
        TUnistat t;
        TTestIniter initer;

        {
            {
                t.Reset();
                initer.Init(t);
                TUnistatExceptionCounter ec{ t, EUnistatTest::Rabbit };
            }
            const auto value = JsonFromString(t.CreateInfoDump(10));
            UNIT_ASSERT_VALUES_EQUAL(value["Rabbit"]["Value"].GetIntegerRobust(), 0);
        }
        {
            {
                t.Reset();
                initer.Init(t);
                TUnistatExceptionCounter ec{ t, EUnistatTest::Rabbit, EUnistatTest::Owl };
            }
            const auto value = JsonFromString(t.CreateInfoDump(10));
            UNIT_ASSERT_VALUES_EQUAL(value["Rabbit"]["Value"].GetIntegerRobust(), 0);
            UNIT_ASSERT_VALUES_EQUAL(value["Owl"]["Value"].GetIntegerRobust(), 1);
        }
        {
            try {
                t.Reset();
                initer.Init(t);
                TUnistatExceptionCounter ec{ t, EUnistatTest::Rabbit };
                ythrow yexception() << "hey, there";
            }
            catch (std::exception&) {
            }
            const auto value = JsonFromString(t.CreateInfoDump(10));
            UNIT_ASSERT_VALUES_EQUAL(value["Rabbit"]["Value"].GetIntegerRobust(), 1);
        }
        {
            try {
                t.Reset();
                initer.Init(t);
                TUnistatExceptionCounter ec{ t, EUnistatTest::Rabbit, EUnistatTest::Owl };
                ythrow yexception() << "hey, there";
            }
            catch (std::exception&) {
            }
            const auto value = JsonFromString(t.CreateInfoDump(10));
            UNIT_ASSERT_VALUES_EQUAL(value["Rabbit"]["Value"].GetIntegerRobust(), 1);
            UNIT_ASSERT_VALUES_EQUAL(value["Owl"]["Value"].GetIntegerRobust(), 0);
        }
        {
            try {
                t.Reset();
                initer.Init(t);
                TUnistatExceptionCounter ec{ t, EUnistatTest::Rabbit };
                ec.Dismiss();
                ythrow yexception() << "hey, there";
            }
            catch (std::exception&) {
            }
            const auto value = JsonFromString(t.CreateInfoDump(10));
            UNIT_ASSERT_VALUES_EQUAL(value["Rabbit"]["Value"].GetIntegerRobust(), 0);
        }
        {
            try {
                t.Reset();
                initer.Init(t);
                TUnistatExceptionCounter ec{ t, EUnistatTest::Rabbit, EUnistatTest::Owl };
                ec.Dismiss();
                ythrow yexception() << "hey, there";
            }
            catch (std::exception&) {
            }
            const auto value = JsonFromString(t.CreateInfoDump(10));
            UNIT_ASSERT_VALUES_EQUAL(value["Rabbit"]["Value"].GetIntegerRobust(), 0);
            UNIT_ASSERT_VALUES_EQUAL(value["Owl"]["Value"].GetIntegerRobust(), 0);
        }
        {
            try {
                t.Reset();
                initer.Init(t);
                TUnistatExceptionCounter ec{ t, EUnistatTest::Rabbit };
                ec.Dismiss();
                ec.Accept();
                ythrow yexception() << "hey, there";
            }
            catch (std::exception&) {
            }
            const auto value = JsonFromString(t.CreateInfoDump(10));
            UNIT_ASSERT_VALUES_EQUAL(value["Rabbit"]["Value"].GetIntegerRobust(), 1);
        }
        {
            try {
                t.Reset();
                initer.Init(t);
                TUnistatExceptionCounter ec{ t, EUnistatTest::Rabbit, EUnistatTest::Owl };
                ec.Dismiss();
                ec.Accept();
                ythrow yexception() << "hey, there";
            }
            catch (std::exception&) {
            }
            const auto value = JsonFromString(t.CreateInfoDump(10));
            UNIT_ASSERT_VALUES_EQUAL(value["Rabbit"]["Value"].GetIntegerRobust(), 1);
            UNIT_ASSERT_VALUES_EQUAL(value["Owl"]["Value"].GetIntegerRobust(), 0);
        }
        {
            {
                t.Reset();
                initer.Init(t);
                Y_UNISTAT_EXCEPTION_COUNTER(t, EUnistatTest::Rabbit)
            }
            const auto value = JsonFromString(t.CreateInfoDump(10));
            UNIT_ASSERT_VALUES_EQUAL(value["Rabbit"]["Value"].GetDoubleRobust(), 0.0);
        }
        {
            {
                t.Reset();
                initer.Init(t);
                Y_UNISTAT_EXCEPTION_COUNTER(t, EUnistatTest::Rabbit, EUnistatTest::Owl)
            }
            const auto value = JsonFromString(t.CreateInfoDump(10));
            UNIT_ASSERT_VALUES_EQUAL(value["Rabbit"]["Value"].GetDoubleRobust(), 0.0);
            UNIT_ASSERT_VALUES_EQUAL(value["Owl"]["Value"].GetDoubleRobust(), 1.0);
        }
    }

    Y_UNIT_TEST(TestSignalValueGetter) {
        TUnistat& t = TUnistat::Instance();
        TTestIniter initer;
        t.Reset();
        initer.Init(t);

        auto value = t.GetSignalValueUnsafe("Rabbit");
        UNIT_ASSERT(!value);
        t.PushSignalUnsafe("Rabbit", 1);
        value = t.GetSignalValueUnsafe("Rabbit");
        UNIT_ASSERT(value);
        UNIT_ASSERT_VALUES_EQUAL(value->GetName().find("Rabbit_"), 0);
        UNIT_ASSERT_VALUES_EQUAL(value->GetNumber(), 1);

        value = t.GetSignalValueUnsafe("Owl");
        UNIT_ASSERT(!value);
        t.PushSignalUnsafe("Owl", 2);
        value = t.GetSignalValueUnsafe("Owl");
        UNIT_ASSERT(value);
        UNIT_ASSERT_VALUES_EQUAL(value->GetName().find("Owl_"), 0);
        UNIT_ASSERT_VALUES_EQUAL(value->GetNumber(), 2);
    }

    Y_UNIT_TEST(TestResetSignal) {
        TUnistat& t = TUnistat::Instance();
        TTestIniter initer;
        t.Reset();
        initer.Init(t);

        auto validateValue = [&t](const TString& holename, double expectedValue) {
            auto value = t.GetSignalValueUnsafe(holename);
            UNIT_ASSERT(value);
            UNIT_ASSERT_VALUES_EQUAL(value->GetName().find(holename + "_"), 0);
            UNIT_ASSERT_VALUES_EQUAL(value->GetNumber(), expectedValue);
        };

        UNIT_ASSERT(t.PushSignalUnsafe("Rabbit", 1));
        UNIT_ASSERT(t.PushSignalUnsafe("Owl", 2));
        validateValue("Rabbit", 1);
        validateValue("Owl", 2);

        UNIT_ASSERT(t.ResetSignalUnsafe("Rabbit"));
        UNIT_ASSERT(!t.GetSignalValueUnsafe("Rabbit"));
        validateValue("Owl", 2);

        UNIT_ASSERT(!t.ResetSignalUnsafe("RandomName"));
        UNIT_ASSERT(!t.GetSignalValueUnsafe("Rabbit"));
        validateValue("Owl", 2);
    }

    Y_UNIT_TEST(TestPushDump) {
        TUnistat& t = TUnistat::Instance();
        TTestIniter initer;
        t.Reset();
        initer.Init(t);
        std::function<void()> exec = []() {
            TUnistat& t = TUnistat::Instance();
            for (size_t i = 1; i < 10; i++) {
                t.PushSignalUnsafe("Shots", i + 0.1);
                t.PushSignalUnsafe("Shots", i * 10 + 0.1);
                t.PushSignalUnsafe("Shots", i * 100 + 0.1);
                t.PushSignalUnsafe("Shots", i * 1000 + 0.1);
            };
        };
        KayakKayak(exec, 100);
        t.PushSignalUnsafe("Owl", 2);
        const auto value = JsonFromString(t.CreatePushDump(10, { { "itype", "my_i_type" },{ "ctype", "testing" } }, 30));
        UNIT_ASSERT_VALUES_EQUAL(value[0]["ttl"].GetUIntegerRobust(), 30);
        UNIT_ASSERT_VALUES_EQUAL(value[0]["tags"]["itype"].GetStringRobust(), "my_i_type");
        UNIT_ASSERT_VALUES_EQUAL(value[0]["tags"]["ctype"].GetStringRobust(), "testing");
        bool wasOwl = false;
        bool wasShots = false;
        for (const auto& signal : value[0]["values"].GetArray()) {
            if (signal["name"].GetStringRobust().StartsWith("Owl_")) {
                UNIT_ASSERT(!wasOwl);
                wasOwl = true;
                UNIT_ASSERT_VALUES_EQUAL(signal["val"], 2);
            } else if (signal["name"].GetStringRobust().StartsWith("Shots_")) {
                UNIT_ASSERT(!wasShots);
                wasShots = true;
                const auto& stats = signal["val"];
                UNIT_ASSERT_VALUES_EQUAL(stats[0][1], 900);
                UNIT_ASSERT_VALUES_EQUAL(stats[1][1], 900);
                UNIT_ASSERT_VALUES_EQUAL(stats[2][1], 900);
                UNIT_ASSERT_VALUES_EQUAL(stats[3][1], 900);
            }
        }
        UNIT_ASSERT(wasShots);
        UNIT_ASSERT(wasOwl);
    }

    Y_UNIT_TEST(TestIntervalsBuilder) {
        NUnistat::TIntervals x;

        x = NUnistat::TIntervalsBuilder().FlatRange(4, 1, 5).FlatRange(3, 5, 20).Build();
        UNIT_ASSERT_VALUES_EQUAL(x.size(), 7);
        UNIT_ASSERT_VALUES_EQUAL(x[0], 1);
        UNIT_ASSERT_VALUES_EQUAL(x[1], 2);
        UNIT_ASSERT_VALUES_EQUAL(x[2], 3);
        UNIT_ASSERT_VALUES_EQUAL(x[3], 4);
        UNIT_ASSERT_VALUES_EQUAL(x[4], 5);
        UNIT_ASSERT_VALUES_EQUAL(x[5], 10);
        UNIT_ASSERT_VALUES_EQUAL(x[6], 15);

        x = NUnistat::TIntervalsBuilder().ExpRange(4, 1, 16).ExpRange(3, 16, 1024).Build();
        UNIT_ASSERT_VALUES_EQUAL(x.size(), 7);
        UNIT_ASSERT_VALUES_EQUAL(x[0], 1);
        UNIT_ASSERT_VALUES_EQUAL(x[1], 2);
        UNIT_ASSERT_VALUES_EQUAL(x[2], 4);
        UNIT_ASSERT_VALUES_EQUAL(x[3], 8);
        UNIT_ASSERT(x[4] - 16 < 0.1);
        UNIT_ASSERT(x[5] - 64 < 0.1);
        UNIT_ASSERT(x[6] - 256 < 0.1);
    }
}
