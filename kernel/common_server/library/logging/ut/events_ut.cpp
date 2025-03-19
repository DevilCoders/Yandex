#include <kernel/common_server/library/logging/events.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/common/network.h>
#include <library/cpp/logger/backend.h>
#include <library/cpp/logger/filter.h>
#include <util/string/vector.h>
#include <kernel/common_server/library/unistat/cache.h>
#include <kernel/common_server/util/logging/tskv_log.h>

Y_UNIT_TEST_SUITE(TestEventsLogSuite) {
    class TStringsLogBackend final: public TLogBackend {
    public:
        TStringsLogBackend(TVector<TString>& strings)
            : Strings(strings)
        {}

        virtual void WriteData(const TLogRecord& rec) override {
            Strings.emplace_back(rec.Data, rec.Len);
        }

        virtual void ReopenLog() override {
        }

    private:
        TVector<TString>& Strings;
    };

    void ResetEventLog(TVector<TString>& strings, ELogPriority priority, NCS::NLogging::ELogRecordFormat format) {
        TLoggerOperator<TFLEventLog>::Set(new TFLEventLog);
        TLoggerOperator<TFLEventLog>::Get()->UsePriorityLimit(priority);
        TLoggerOperator<TFLEventLog>::Get()->ResetBackend(MakeHolder<TFilteredLogBackend>(MakeHolder<TStringsLogBackend>(strings), priority));
        TFLEventLog::SetFormat(format);
    }

    Y_UNIT_TEST(TestMultipleFieldValues) {
        TVector<TString> strings;
        ResetEventLog(strings, ELogPriority::TLOG_INFO, NCS::NLogging::ELogRecordFormat::TSKV);

        {
            auto l = TFLEventLog::Info("test")("a", 1);
            l("b", 2);
            l("c", 3);
            l("*c", 4);
            l("c", 5);
            l("a", 8);
            l("*b", 1);
            l("a", 7);
        }
        UNIT_ASSERT_EQUAL(strings.size(), 1);
        TMap<TString, TString> values;
        NUtil::TTSKVRecordParser::Parse(strings[0], values);
        UNIT_ASSERT_EQUAL(values["a"], "7");
        UNIT_ASSERT_EQUAL(values["b"], "1");
        UNIT_ASSERT_EQUAL(values["c"], "4");
    }

    Y_UNIT_TEST(TestMultipleFieldValuesSignal) {
        TVector<TString> strings;
        ResetEventLog(strings, ELogPriority::TLOG_INFO, NCS::NLogging::ELogRecordFormat::TSKV);
        TMonitoringConfig config;
        auto holder = NTesting::GetFreePort();
        config.MutableHttpOptions().SetPort((ui16)holder);
        TCSSignals::RegisterMonitoring(config);

        auto gLogging = TFLRecords::StartContext().SignalId("test_signal")("&b", 1);
        {
            auto gLogging = TFLRecords::StartContext().SignalId("test_signal1")("&a", 2);
            TFLEventLog::Signal("test1", 3)("&a", 1);
            TFLEventLog::Signal("", 4)("&a", 3);
            TFLEventLog::LTSignal("", 5)("&c", 3);
            TFLEventLog::Signal();
        }
        UNIT_ASSERT_EQUAL(strings.size(), 4);

        {
            NMonitoring::TLabels labels;
            labels.Add("a", "1");
            labels.Add("b", "1");
            labels.Add("sensor", "test1");
            UNIT_ASSERT_EQUAL_C(TCSSignals::GetValueRate(labels), 3, TCSSignals::GetValueRate(labels));
        }
        {
            NMonitoring::TLabels labels;
            labels.Add("priority", "INFO");
            labels.Add("sensor", "event_log");
            UNIT_ASSERT_EQUAL_C(TCSSignals::GetValueRate(labels), 4, TCSSignals::GetValueRate(labels));
        }
        {
            NMonitoring::TLabels labels;
            labels.Add("a", "2");
            labels.Add("b", "1");
            labels.Add("c", "3");
            labels.Add("sensor", "test_signal1");
            UNIT_ASSERT_EQUAL_C(TCSSignals::GetValueGauge(labels), 5, TCSSignals::GetValueGauge(labels));
        }
        {
            NMonitoring::TLabels labels;
            labels.Add("a", "2");
            labels.Add("b", "1");
            labels.Add("sensor", "test_signal1");
            UNIT_ASSERT_EQUAL_C(TCSSignals::GetValueRate(labels), 1, TCSSignals::GetValueRate(labels));
        }
        {
            NMonitoring::TLabels labels;
            labels.Add("a", "3");
            labels.Add("b", "1");
            labels.Add("sensor", "test_signal1");
            UNIT_ASSERT_EQUAL_C(TCSSignals::GetValueRate(labels), 4, TCSSignals::GetValueRate(labels));
        }
        TCSSignals::UnregisterMonitoring();
    }

    Y_UNIT_TEST(TestSignalIdProviding) {
        TVector<TString> strings;
        ResetEventLog(strings, ELogPriority::TLOG_INFO, NCS::NLogging::ELogRecordFormat::TSKV);
        TLoggerOperator<TFLEventLog>::Get()->ActivateBackgroundWriting(512, false);
        TMonitoringConfig config;
        auto holder = NTesting::GetFreePort();
        config.MutableHttpOptions().SetPort((ui16)holder);
        TCSSignals::RegisterMonitoring(config);

        auto gLogging = TFLRecords::StartContext().SignalId("abcde");
        auto gLoggingf = TFLRecords::StartContext().SignalId("abcdef");
        TFLEventLog::JustSignal();

        NMonitoring::TLabels labels;
        labels.Add("sensor", "abcdef");
        CHECK_WITH_LOG(TFLEventLog::WaitQueueEmpty(Now() + TDuration::Seconds(10)));
        CHECK_WITH_LOG(TCSSignals::GetValueRate(labels) == 1) << TCSSignals::GetValueRate(labels);
        CHECK_WITH_LOG(strings.size() == 0) << strings.size();
        TCSSignals::UnregisterMonitoring();
    }

    Y_UNIT_TEST(TestNativePrioritySignalBottom) {
        TVector<TString> strings;
        ResetEventLog(strings, ELogPriority::TLOG_INFO, NCS::NLogging::ELogRecordFormat::TSKV);
        TLoggerOperator<TFLEventLog>::Get()->ActivateBackgroundWriting(512, false);
        TMonitoringConfig config;
        auto holder = NTesting::GetFreePort();
        config.MutableHttpOptions().SetPort((ui16)holder);
        TCSSignals::RegisterMonitoring(config);

        TFLEventLog::JustSignal("test", 123);
        NMonitoring::TLabels labels;
        labels.Add("sensor", "test");
        CHECK_WITH_LOG(TFLEventLog::WaitQueueEmpty(Now() + TDuration::Seconds(10)));
        CHECK_WITH_LOG(TCSSignals::GetValueRate(labels) == 123) << TCSSignals::GetValueRate(labels);
        CHECK_WITH_LOG(strings.size() == 0) << strings.size();
        TCSSignals::UnregisterMonitoring();
    }

    Y_UNIT_TEST(TestNativePrioritySignalWrite) {
        TVector<TString> strings;
        ResetEventLog(strings, ELogPriority::TLOG_RESOURCES, NCS::NLogging::ELogRecordFormat::TSKV);
        TLoggerOperator<TFLEventLog>::Get()->ActivateBackgroundWriting(512, false);
        TMonitoringConfig config;
        auto holder = NTesting::GetFreePort();
        config.MutableHttpOptions().SetPort((ui16)holder);
        TCSSignals::RegisterMonitoring(config);

        TFLEventLog::JustSignal("test", 123);
        NMonitoring::TLabels labels;
        labels.Add("sensor", "test");
        CHECK_WITH_LOG(TFLEventLog::WaitQueueEmpty(Now() + TDuration::Seconds(10)));
        CHECK_WITH_LOG(TCSSignals::GetValueRate(labels) == 123) << TCSSignals::GetValueRate(labels);
        CHECK_WITH_LOG(strings.size() == 1) << strings.size();
        TCSSignals::UnregisterMonitoring();
    }

    void TestLevel(ELogPriority level, NCS::NLogging::ELogRecordFormat format) {
        static const TMap<ELogPriority, ui32> expectedCount = {
            { TLOG_EMERG, 3 },
            { TLOG_ALERT, 3 },
            { TLOG_CRIT, 5 },
            { TLOG_ERR, 5 },
            { TLOG_WARNING, 5 },
            { TLOG_NOTICE, 5 },
            { TLOG_INFO, 7 },
            { TLOG_DEBUG, 5 }
        };
        TVector<TString> strings;
        ResetEventLog(strings, level, format);
        TFLEventLog::Log()("msg", "default Log");
        TFLEventLog::Log("default text")("msg", "Log(text)");
        for (ui32 i = 0; i < (ui32)LOG_MAX_PRIORITY; ++i) {
            ELogPriority p = ELogPriority(i);
            TFLEventLog::Log(ToString(p) + " text", p)("msg", "Log(text, " + ToString(p) + ")");
            NCS::NLogging::TBaseLogRecord r;
            r.SetPriority(p);
            r.Add("record", ToString(p));
            TFLEventLog::Log(r)("msg", "Log(const record&)");
            TFLEventLog::Log(std::move(r))("msg", "Log(record&&)");
        }
        TFLEventLog::Debug("Debug text")("msg", "Debug");
        TFLEventLog::Debug()("msg", "Debug empty");

        TFLEventLog::Info("Info text")("msg", "Info");
        TFLEventLog::Info()("msg", "Info empty");

        TFLEventLog::Notice("Notice text")("msg", "Notice");
        TFLEventLog::Notice()("msg", "Notice empty");

        TFLEventLog::Warning("Warning text")("msg", "Warning");
        TFLEventLog::Warning()("msg", "Warning empty");

        TFLEventLog::Error("Error text")("msg", "Error");
        TFLEventLog::Error()("msg", "Error empty");

        TFLEventLog::Critical("Critical text")("msg", "Critical");
        TFLEventLog::Critical()("msg", "Critical empty");

        ui32 expected = 0;
        for (ui32 i = 0; i <= level; ++i) {
            expected += expectedCount.find(ELogPriority(i))->second;
        }
        UNIT_ASSERT_EQUAL_C(strings.size(), expected, JoinVectorIntoString(strings, ""));
    }

    Y_UNIT_TEST(TestEmergTskv) {
        TestLevel(TLOG_EMERG, NCS::NLogging::ELogRecordFormat::TSKV);
    }
    Y_UNIT_TEST(TestAlertTskv) {
        TestLevel(TLOG_ALERT, NCS::NLogging::ELogRecordFormat::TSKV);
    }
    Y_UNIT_TEST(TestCritTskv) {
        TestLevel(TLOG_CRIT, NCS::NLogging::ELogRecordFormat::TSKV);
    }
    Y_UNIT_TEST(TestErrorTskv) {
        TestLevel(TLOG_ERR, NCS::NLogging::ELogRecordFormat::TSKV);
    }
    Y_UNIT_TEST(TestWarrningTskv) {
        TestLevel(TLOG_WARNING, NCS::NLogging::ELogRecordFormat::TSKV);
    }
    Y_UNIT_TEST(TestNoticeTskv) {
        TestLevel(TLOG_NOTICE, NCS::NLogging::ELogRecordFormat::TSKV);
    }
    Y_UNIT_TEST(TestInfoTskv) {
        TestLevel(TLOG_INFO, NCS::NLogging::ELogRecordFormat::TSKV);
    }
    Y_UNIT_TEST(TestDebugTskv) {
        TestLevel(TLOG_DEBUG, NCS::NLogging::ELogRecordFormat::TSKV);
    }

    Y_UNIT_TEST(TestEmergJson) {
        TestLevel(TLOG_EMERG, NCS::NLogging::ELogRecordFormat::Json);
    }
    Y_UNIT_TEST(TestAlertJson) {
        TestLevel(TLOG_ALERT, NCS::NLogging::ELogRecordFormat::Json);
    }
    Y_UNIT_TEST(TestCritJson) {
        TestLevel(TLOG_CRIT, NCS::NLogging::ELogRecordFormat::Json);
    }
    Y_UNIT_TEST(TestErrorJson) {
        TestLevel(TLOG_ERR, NCS::NLogging::ELogRecordFormat::Json);
    }
    Y_UNIT_TEST(TestWarrningJson) {
        TestLevel(TLOG_WARNING, NCS::NLogging::ELogRecordFormat::Json);
    }
    Y_UNIT_TEST(TestNoticeJson) {
        TestLevel(TLOG_NOTICE, NCS::NLogging::ELogRecordFormat::Json);
    }
    Y_UNIT_TEST(TestInfoJson) {
        TestLevel(TLOG_INFO, NCS::NLogging::ELogRecordFormat::Json);
    }
    Y_UNIT_TEST(TestDebugJson) {
        TestLevel(TLOG_DEBUG, NCS::NLogging::ELogRecordFormat::Json);
    }

    Y_UNIT_TEST(TestEmergHR) {
        TestLevel(TLOG_EMERG, NCS::NLogging::ELogRecordFormat::HR);
    }
    Y_UNIT_TEST(TestAlertHR) {
        TestLevel(TLOG_ALERT, NCS::NLogging::ELogRecordFormat::HR);
    }
    Y_UNIT_TEST(TestCritHR) {
        TestLevel(TLOG_CRIT, NCS::NLogging::ELogRecordFormat::HR);
    }
    Y_UNIT_TEST(TestErrorHR) {
        TestLevel(TLOG_ERR, NCS::NLogging::ELogRecordFormat::HR);
    }
    Y_UNIT_TEST(TestWarrningHR) {
        TestLevel(TLOG_WARNING, NCS::NLogging::ELogRecordFormat::HR);
    }
    Y_UNIT_TEST(TestNoticeHR) {
        TestLevel(TLOG_NOTICE, NCS::NLogging::ELogRecordFormat::HR);
    }
    Y_UNIT_TEST(TestInfoHR) {
        TestLevel(TLOG_INFO, NCS::NLogging::ELogRecordFormat::HR);
    }
    Y_UNIT_TEST(TestDebugHR) {
        TestLevel(TLOG_DEBUG, NCS::NLogging::ELogRecordFormat::HR);
    }

};
