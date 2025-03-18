#include <library/cpp/trace_usage/context.h>
#include <library/cpp/trace_usage/function_scope.h>
#include <library/cpp/trace_usage/global_registry.h>
#include <library/cpp/trace_usage/logger.h>
#include <library/cpp/trace_usage/logreader.h>
#include <library/cpp/trace_usage/protos/event.pb.h>
#include <library/cpp/trace_usage/traced_guard.h>
#include <library/cpp/trace_usage/wait_scope.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/stream/buffer.h>
#include <util/system/event.h>
#include <util/system/mutex.h>
#include <util/system/thread.h>
#include <util/generic/ptr.h>

using namespace NTraceUsage;

namespace {
    TMutex TraceUsageSingletonTestMutex;
    TManualEvent GlobalTestEvent;

    void* SignalEventThreadProcedure(void*) {
        Sleep(TDuration::MilliSeconds(100));
        GlobalTestEvent.Signal();
        return nullptr;
    }

    void TracedFunction() {
        TFunctionScope functionScope(TStringBuf("TracedFunction"));
    }

    TStringBuf longFunctionName(
        "TracedLongNameFunction"
        "0123456789"
        "0123456789"
        "0123456789"
        "0123456789"
        "0123456789" // 50 chars
        "0123456789"
        "0123456789"
        "0123456789"
        "0123456789"
        "0123456789" // 50 chars
        "0123456789"
        "0123456789"
        "0123456789"
        "0123456789"
        "0123456789" // 50 chars
        "0123456789"
        "0123456789"
        "0123456789"
        "0123456789"
        "0123456789" // 50 chars
    );

    void TracedLongNameFunction() {
        TFunctionScope functionScope(longFunctionName);
    }

    void CheckCommon(const TEventReportProto& eventReport) {
        UNIT_ASSERT(eventReport.HasCommonEventData());
        auto& commonData = eventReport.GetCommonEventData();
        UNIT_ASSERT(commonData.HasMicroSecondsTime());
        UNIT_ASSERT(commonData.HasThreadId());
    }

    void IsSameThread(const TEventReportProto& left, const TEventReportProto& right) {
        UNIT_ASSERT_EQUAL(left.GetCommonEventData().GetThreadId(), right.GetCommonEventData().GetThreadId());
    }
    template <typename... TArgs>
    void IsSameThread(const TEventReportProto& left, const TEventReportProto& right, TArgs&&... args) {
        IsSameThread(left, right);
        IsSameThread(right, std::forward<TArgs>(args)...);
    }

    void IsAfter(const TEventReportProto& left, const TEventReportProto& right) {
        UNIT_ASSERT_C(left.GetCommonEventData().GetMicroSecondsTime() <= right.GetCommonEventData().GetMicroSecondsTime(), "Wrong time order!");
    }
    template <typename... TArgs>
    void IsAfter(const TEventReportProto& left, const TEventReportProto& right, TArgs&&... args) {
        IsAfter(left, right);
        IsAfter(right, std::forward<TArgs>(args)...);
    }

    void IsFunctionName(TStringBuf functionName, const TFunctionScopeEvent& functionEvent) {
        UNIT_ASSERT(functionEvent.HasFunction());
        UNIT_ASSERT_EQUAL(functionEvent.GetFunction(), functionName);
    }
    template <typename... TArgs>
    void IsFunctionName(TStringBuf functionName, const TFunctionScopeEvent& functionEvent, TArgs&&... args) {
        IsFunctionName(functionName, functionEvent);
        IsFunctionName(functionName, std::forward<TArgs>(args)...);
    }

    void IsParamName(const TEventParam& eventParam, TStringBuf name) {
        UNIT_ASSERT_EQUAL(eventParam.NameSize(), 1);
        UNIT_ASSERT_EQUAL(eventParam.GetName(0), name);
    }

}

Y_UNIT_TEST_SUITE(TraceUsageTests) {
    Y_UNIT_TEST(TracedFunctionTest) {
        auto guard = Guard(TraceUsageSingletonTestMutex);
        TBuffer buf;
        {
            TTraceRegistryPtr logger = new TUsageLogger(MakeHolder<TBufferStream>(buf));
            TGlobalRegistryGuard global(logger);
            TracedFunction();
        }
        {
            TLogReader logReader(MakeHolder<TBufferInput>(buf));
            TEventReportProto eventReport;

            {
                // Message 1
                bool hasMessage1 = logReader.Next(eventReport);
                UNIT_ASSERT(hasMessage1);
                CheckCommon(eventReport);
                UNIT_ASSERT_EQUAL(eventReport.StartFunctionScopeSize(), 1);
                UNIT_ASSERT_EQUAL(eventReport.CloseFunctionScopeSize(), 0);
            }
            TEventReportProto message1 = eventReport;

            {
                // Message 2
                bool hasMessage2 = logReader.Next(eventReport);
                UNIT_ASSERT(hasMessage2);
                CheckCommon(eventReport);
                UNIT_ASSERT_EQUAL(eventReport.StartFunctionScopeSize(), 0);
                UNIT_ASSERT_EQUAL(eventReport.CloseFunctionScopeSize(), 1);
            }
            TEventReportProto message2 = eventReport;

            {
                // No message 3
                bool hasMessage3 = logReader.Next(eventReport);
                UNIT_ASSERT(!hasMessage3);
            }

            {
                // Correctness
                IsSameThread(message1, message2);
                IsAfter(message1, message2);
                IsFunctionName(TStringBuf("TracedFunction"), message1.GetStartFunctionScope(0), message2.GetCloseFunctionScope(0));
            }
        }
    }
    Y_UNIT_TEST(TracedLongNameFunctionTest) {
        auto guard = Guard(TraceUsageSingletonTestMutex);
        TBuffer buf;
        {
            TTraceRegistryPtr logger = new TUsageLogger(MakeHolder<TBufferStream>(buf));
            TGlobalRegistryGuard global(logger);
            TracedLongNameFunction();
        }
        {
            TLogReader logReader(MakeHolder<TBufferInput>(buf));
            TEventReportProto eventReport;

            {
                // Message 1
                bool hasMessage1 = logReader.Next(eventReport);
                UNIT_ASSERT(hasMessage1);
                CheckCommon(eventReport);
                UNIT_ASSERT_EQUAL(eventReport.StartFunctionScopeSize(), 1);
                UNIT_ASSERT_EQUAL(eventReport.CloseFunctionScopeSize(), 0);
            }
            TEventReportProto message1 = eventReport;

            {
                // Message 2
                bool hasMessage2 = logReader.Next(eventReport);
                UNIT_ASSERT(hasMessage2);
                CheckCommon(eventReport);
                UNIT_ASSERT_EQUAL(eventReport.StartFunctionScopeSize(), 0);
                UNIT_ASSERT_EQUAL(eventReport.CloseFunctionScopeSize(), 1);
            }
            TEventReportProto message2 = eventReport;

            {
                // No message 3
                bool hasMessage3 = logReader.Next(eventReport);
                UNIT_ASSERT(!hasMessage3);
            }

            {
                // Correctness
                IsSameThread(message1, message2);
                IsAfter(message1, message2);
                IsFunctionName(longFunctionName, message1.GetStartFunctionScope(0), message2.GetCloseFunctionScope(0));
            }
        }
    }
    Y_UNIT_TEST(TracedMutexTest) {
        auto guard = Guard(TraceUsageSingletonTestMutex);
        TBuffer buf;
        TMutex testMutex;
        {
            TTraceRegistryPtr logger = new TUsageLogger(MakeHolder<TBufferStream>(buf));
            TGlobalRegistryGuard global(logger);
            {
                TTracedGuard<TMutex> guard2(testMutex);
            }
        }
        {
            TLogReader logReader(MakeHolder<TBufferInput>(buf));
            TEventReportProto eventReport;

            {
                // Message 1
                bool hasMessage1 = logReader.Next(eventReport);
                UNIT_ASSERT(hasMessage1);
                CheckCommon(eventReport);
                UNIT_ASSERT_EQUAL(eventReport.StartAcquiringMutexSize(), 1);
                UNIT_ASSERT_EQUAL(eventReport.AcquiredMutexSize(), 0);
                UNIT_ASSERT_EQUAL(eventReport.StartReleasingMutexSize(), 0);
                UNIT_ASSERT_EQUAL(eventReport.ReleasedMutexSize(), 0);
            }
            TEventReportProto message1 = eventReport;

            {
                // Message 2
                bool hasMessage2 = logReader.Next(eventReport);
                UNIT_ASSERT(hasMessage2);
                CheckCommon(eventReport);
                UNIT_ASSERT_EQUAL(eventReport.StartAcquiringMutexSize(), 0);
                UNIT_ASSERT_EQUAL(eventReport.AcquiredMutexSize(), 1);
                UNIT_ASSERT_EQUAL(eventReport.StartReleasingMutexSize(), 0);
                UNIT_ASSERT_EQUAL(eventReport.ReleasedMutexSize(), 0);
            }
            TEventReportProto message2 = eventReport;

            {
                // Message 3
                bool hasMessage3 = logReader.Next(eventReport);
                UNIT_ASSERT(hasMessage3);
                CheckCommon(eventReport);
                UNIT_ASSERT_EQUAL(eventReport.StartAcquiringMutexSize(), 0);
                UNIT_ASSERT_EQUAL(eventReport.AcquiredMutexSize(), 0);
                UNIT_ASSERT_EQUAL(eventReport.StartReleasingMutexSize(), 1);
                UNIT_ASSERT_EQUAL(eventReport.ReleasedMutexSize(), 0);
            }
            TEventReportProto message3 = eventReport;

            {
                // Message 4
                bool hasMessage4 = logReader.Next(eventReport);
                UNIT_ASSERT(hasMessage4);
                CheckCommon(eventReport);
                UNIT_ASSERT_EQUAL(eventReport.StartAcquiringMutexSize(), 0);
                UNIT_ASSERT_EQUAL(eventReport.AcquiredMutexSize(), 0);
                UNIT_ASSERT_EQUAL(eventReport.StartReleasingMutexSize(), 0);
                UNIT_ASSERT_EQUAL(eventReport.ReleasedMutexSize(), 1);
            }
            TEventReportProto message4 = eventReport;

            {
                // No message 5
                bool hasMessage5 = logReader.Next(eventReport);
                UNIT_ASSERT(!hasMessage5);
            }

            {
                // Correctness
                IsSameThread(message1, message2, message3, message4);
                IsAfter(message1, message2, message3, message4);
            }
        }
    }

    Y_UNIT_TEST(TracedWaitEvent) {
        auto guard = Guard(TraceUsageSingletonTestMutex);
        TBuffer buf;
        GlobalTestEvent.Reset();
        TThread thread(&SignalEventThreadProcedure, nullptr);
        thread.Start();
        {
            TTraceRegistryPtr logger = new TUsageLogger(MakeHolder<TBufferStream>(buf));
            TGlobalRegistryGuard global(logger);
            {
                TWaitScope waitScope;
                GlobalTestEvent.Wait();
            }
        }
        thread.Join();
        {
            TLogReader logReader(MakeHolder<TBufferInput>(buf));
            TEventReportProto eventReport;

            {
                // Message 1
                bool hasMessage1 = logReader.Next(eventReport);
                UNIT_ASSERT(hasMessage1);
                CheckCommon(eventReport);
                UNIT_ASSERT_EQUAL(eventReport.StartWaitEventSize(), 1);
                UNIT_ASSERT_EQUAL(eventReport.FinishWaitEventSize(), 0);
            }
            TEventReportProto message1 = eventReport;

            {
                // Message 2
                bool hasMessage2 = logReader.Next(eventReport);
                UNIT_ASSERT(hasMessage2);
                CheckCommon(eventReport);
                UNIT_ASSERT_EQUAL(eventReport.StartWaitEventSize(), 0);
                UNIT_ASSERT_EQUAL(eventReport.FinishWaitEventSize(), 1);
            }
            TEventReportProto message2 = eventReport;

            {
                // No message 3
                bool hasMessage3 = logReader.Next(eventReport);
                UNIT_ASSERT(!hasMessage3);
            }

            {
                // Correctness
                IsSameThread(message1, message2);
                IsAfter(message1, message2);
            }
        }
    }

    Y_UNIT_TEST(TracedInplaceUserDefinedMessage) {
        for (bool moveType : {false, true}) {
            auto guard = Guard(TraceUsageSingletonTestMutex);
            TBuffer buf;
            float testFloatValue = 0.123;
            double testDoubleValue = 0.321;
            const TStringBuf factorNames[] = {
                TStringBuf("FirstFactor"),
                TStringBuf("SecondFactor")};
            float factors[] = {
                0.1,
                0.2};
            {
                TTraceRegistryPtr logger = new TUsageLogger(MakeHolder<TBufferStream>(buf));
                TGlobalRegistryGuard global(logger);

                using TReportConstructor = NTraceUsage::ITraceRegistry::TReportConstructor;

                TReportConstructor initReport;
                THolder<TReportConstructor> movedToReport;

                auto* report = &initReport;

                report->AddTag(TStringBuf("SomeTag"))
                    .AddParam(TStringBuf("StringParam"), TStringBuf("StringValue"))
                    .AddParam(TStringBuf("UintParam"), (ui64)123)
                    .AddParam(TStringBuf("IntParam"), (i64)-123);

                // pass control to second report
                if (moveType) {
                    movedToReport = MakeHolder<TReportConstructor>(std::move(initReport));
                    report = movedToReport.Get();
                }

                report->AddParam(TStringBuf("FloatParam"), testFloatValue);

                report->AddParam(TStringBuf("DoubleParam"), testDoubleValue)
                    .AddFactors(factorNames, factors);
            }
            {
                TLogReader logReader(MakeHolder<TBufferInput>(buf));
                TEventReportProto eventReport;

                {
                    // Message 1
                    bool hasMessage1 = logReader.Next(eventReport);
                    UNIT_ASSERT(hasMessage1);
                    CheckCommon(eventReport);
                    UNIT_ASSERT_EQUAL(eventReport.EventParamsSize(), 7); // tag + 5 params + factors

                    // Tag
                    auto& tag = eventReport.GetEventParams(0);
                    IsParamName(tag, TStringBuf("SomeTag"));

                    // String param
                    auto& stringParam = eventReport.GetEventParams(1);
                    IsParamName(stringParam, TStringBuf("StringParam"));
                    UNIT_ASSERT_EQUAL(stringParam.GetStringValue(), TStringBuf("StringValue"));

                    // Unsigned integer param
                    auto& uintParam = eventReport.GetEventParams(2);
                    IsParamName(uintParam, TStringBuf("UintParam"));
                    UNIT_ASSERT_EQUAL(uintParam.GetUintValue(), (ui64)123);

                    // Signed integer param
                    auto& sintParam = eventReport.GetEventParams(3);
                    IsParamName(sintParam, TStringBuf("IntParam"));
                    UNIT_ASSERT_EQUAL(sintParam.GetSintValue(), (i64)-123);

                    // Float param
                    auto& floatParam = eventReport.GetEventParams(4);
                    IsParamName(floatParam, TStringBuf("FloatParam"));
                    UNIT_ASSERT_EQUAL(floatParam.FloatValueSize(), 1);
                    UNIT_ASSERT_EQUAL(floatParam.GetFloatValue(0), testFloatValue);

                    // Double param
                    auto& doubleParam = eventReport.GetEventParams(5);
                    IsParamName(doubleParam, TStringBuf("DoubleParam"));
                    UNIT_ASSERT_EQUAL(doubleParam.DoubleValueSize(), 1);
                    UNIT_ASSERT_EQUAL(doubleParam.GetDoubleValue(0), testDoubleValue);

                    // Factors param
                    auto& factorsParam = eventReport.GetEventParams(6);
                    UNIT_ASSERT_EQUAL(factorsParam.NameSize(), 2);
                    UNIT_ASSERT_EQUAL(factorsParam.GetName(0), factorNames[0]);
                    UNIT_ASSERT_EQUAL(factorsParam.GetName(1), factorNames[1]);
                    UNIT_ASSERT_EQUAL(factorsParam.FloatValueSize(), 2);
                    UNIT_ASSERT_EQUAL(factorsParam.GetFloatValue(0), factors[0]);
                    UNIT_ASSERT_EQUAL(factorsParam.GetFloatValue(1), factors[1]);
                }

                {
                    // No message 2
                    bool hasMessage2 = logReader.Next(eventReport);
                    UNIT_ASSERT(!hasMessage2);
                }
            }
        }
    }

    Y_UNIT_TEST(ReportContext) {
        TBuffer buf;
        auto context = TReportContext::New();

        {
            TTraceRegistryPtr logger = new TUsageLogger(MakeHolder<TBufferStream>(buf));
            TGlobalRegistryGuard global(logger);

            context.NewReport()
                .AddTag(TStringBuf("SomeTag"))
                .AddParam(TStringBuf("StringParam"), TStringBuf("StringValue"));
        }
        {
            TLogReader logReader(MakeHolder<TBufferInput>(buf));
            TEventReportProto eventReport;

            {
                // Message 1
                bool hasMessage1 = logReader.Next(eventReport);
                UNIT_ASSERT(hasMessage1);
                CheckCommon(eventReport);
                UNIT_ASSERT_EQUAL(eventReport.EventParamsSize(), 2); // tag + param

                UNIT_ASSERT_VALUES_EQUAL(eventReport.GetCommonEventData().GetContextId(), context.Id());

                // Tag
                auto& tag = eventReport.GetEventParams(0);
                IsParamName(tag, TStringBuf("SomeTag"));

                // String param
                auto& stringParam = eventReport.GetEventParams(1);
                IsParamName(stringParam, TStringBuf("StringParam"));
                UNIT_ASSERT_EQUAL(stringParam.GetStringValue(), TStringBuf("StringValue"));
            }
            {
                // no Message 2
                UNIT_ASSERT(!logReader.Next(eventReport));
            }
        }
    }

    Y_UNIT_TEST(ChildContext) {
        TBuffer buf;
        // test lazyness
        const auto context = TReportContext::New();
        TMaybe<TReportContext> child;

        {
            TTraceRegistryPtr logger = new TUsageLogger(MakeHolder<TBufferStream>(buf));
            TGlobalRegistryGuard global(logger);

            child = context.NewChildContext();

            child->NewReport()
                .AddTag(TStringBuf("Dummy"));
        }

        {
            TLogReader logReader(MakeHolder<TBufferInput>(buf));
            TEventReportProto eventReport;

            {
                // Message 1
                bool hasMessage1 = logReader.Next(eventReport);
                UNIT_ASSERT(hasMessage1);
                CheckCommon(eventReport);

                UNIT_ASSERT(eventReport.GetCommonEventData().HasContextId());
                UNIT_ASSERT_VALUES_EQUAL(eventReport.GetCommonEventData().GetContextId(), context.Id());

                UNIT_ASSERT(eventReport.HasChildContextDeclaration());
                UNIT_ASSERT_VALUES_EQUAL(eventReport.GetChildContextDeclaration().GetContextId(), child->Id());
            }
            {
                // Message 2
                UNIT_ASSERT(logReader.Next(eventReport));

                UNIT_ASSERT_VALUES_EQUAL(eventReport.GetCommonEventData().GetContextId(), child->Id());

                // Tag
                auto& tag = eventReport.GetEventParams(0);
                IsParamName(tag, TStringBuf("Dummy"));
            }
            {
                // no Message 3
                UNIT_ASSERT(!logReader.Next(eventReport));
            }
        }
    }

    Y_UNIT_TEST(ChildFifo) {
        TBuffer buf;
        // test lazyness
        const auto context = TReportContext::New();
        TMaybe<TReportContext> child;
        TMaybe<TReportContext> grandchild;

        {
            TTraceRegistryPtr logger = new TUsageLogger(MakeHolder<TBufferStream>(buf));
            TGlobalRegistryGuard global(logger);

            child = context.NewChildContext();
            grandchild = child->NewChildContext();

            grandchild->NewReport()
                .AddTag(TStringBuf("Dummy"));
        }

        {
            TLogReader logReader(MakeHolder<TBufferInput>(buf));
            TEventReportProto eventReport;

            {
                // Message 1
                bool hasMessage1 = logReader.Next(eventReport);
                UNIT_ASSERT(hasMessage1);
                CheckCommon(eventReport);

                UNIT_ASSERT(eventReport.GetCommonEventData().HasContextId());
                UNIT_ASSERT_VALUES_EQUAL(eventReport.GetCommonEventData().GetContextId(), context.Id());

                UNIT_ASSERT(eventReport.HasChildContextDeclaration());
                UNIT_ASSERT_VALUES_EQUAL(eventReport.GetChildContextDeclaration().GetContextId(), child->Id());
            }
            {
                // Message 2
                bool hasMessage1 = logReader.Next(eventReport);
                UNIT_ASSERT(hasMessage1);
                CheckCommon(eventReport);

                UNIT_ASSERT(eventReport.GetCommonEventData().HasContextId());
                UNIT_ASSERT_VALUES_EQUAL(eventReport.GetCommonEventData().GetContextId(), child->Id());

                UNIT_ASSERT(eventReport.HasChildContextDeclaration());
                UNIT_ASSERT_VALUES_EQUAL(eventReport.GetChildContextDeclaration().GetContextId(), grandchild->Id());
            }
            {
                // Message 3
                UNIT_ASSERT(logReader.Next(eventReport));

                UNIT_ASSERT_VALUES_EQUAL(eventReport.GetCommonEventData().GetContextId(), grandchild->Id());

                // Tag
                auto& tag = eventReport.GetEventParams(0);
                IsParamName(tag, TStringBuf("Dummy"));
            }
            {
                // no Message 4
                UNIT_ASSERT(!logReader.Next(eventReport));
            }
        }
    }
}
