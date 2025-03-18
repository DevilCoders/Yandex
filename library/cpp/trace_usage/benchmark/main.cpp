#include <library/cpp/testing/benchmark/bench.h>

#include <library/cpp/trace_usage/context.h>
#include <library/cpp/trace_usage/function_scope.h>
#include <library/cpp/trace_usage/global_registry.h>
#include <library/cpp/trace_usage/logger.h>
#include <library/cpp/trace_usage/traced_guard.h>

#include <util/generic/xrange.h>
#include <util/stream/buffer.h>
#include <util/stream/file.h>
#include <util/system/guard.h>
#include <util/system/mutex.h>

using namespace NTraceUsage;

Y_NO_INLINE void fn_empty(size_t i) {
    Y_DO_NOT_OPTIMIZE_AWAY(i);
}

Y_NO_INLINE void fn_empty_traced(size_t i) {
    TFunctionScope(TStringBuf(__PRETTY_FUNCTION__));

    Y_DO_NOT_OPTIMIZE_AWAY(i);
}

// No tracing
Y_CPU_BENCHMARK(EmptyFunction, iface) {
    for (size_t i : xrange(iface.Iterations())) {
        fn_empty(i);
    }
}

// Trace but discard
Y_CPU_BENCHMARK(NullTracedEmptyFunction, iface) {
    for (size_t i : xrange(iface.Iterations())) {
        fn_empty_traced(i);
    }
}

// Trace and save
Y_CPU_BENCHMARK(TracedEmptyFunction, iface) {
    TTraceRegistryPtr logger = new TUsageLogger(MakeHolder<TBufferStream>());
    TGlobalRegistryGuard global(logger);

    for (size_t i : xrange(iface.Iterations())) {
        fn_empty_traced(i);
    }
}
Y_CPU_BENCHMARK(FileTracedEmptyFunction, iface) {
    TTraceRegistryPtr logger = new TUsageLogger(MakeHolder<TOFStream>("bench.trace"));
    TGlobalRegistryGuard global(logger);

    for (size_t i : xrange(iface.Iterations())) {
        fn_empty_traced(i);
    }
}
Y_CPU_BENCHMARK(ZipFileTracedEmptyFunction, iface) {
    TTraceRegistryPtr logger = new TUsageLogger("bench.trace");
    TGlobalRegistryGuard global(logger);

    for (size_t i : xrange(iface.Iterations())) {
        fn_empty_traced(i);
    }
}

// No tracing
Y_CPU_BENCHMARK(Mutex, iface) {
    TMutex mutex;

    for (size_t i : xrange(iface.Iterations())) {
        auto guard = Guard(mutex);
        Y_DO_NOT_OPTIMIZE_AWAY(i);
    }
}

// Trace but discard
Y_CPU_BENCHMARK(NullTracedMutex, iface) {
    TMutex mutex;

    for (size_t i : xrange(iface.Iterations())) {
        TTracedGuard<TMutex> guard(mutex);
        Y_DO_NOT_OPTIMIZE_AWAY(i);
    }
}

// Trace and save
Y_CPU_BENCHMARK(TracedMutex, iface) {
    TTraceRegistryPtr logger = new TUsageLogger(MakeHolder<TBufferStream>());
    TGlobalRegistryGuard global(logger);

    TMutex mutex;

    for (size_t i : xrange(iface.Iterations())) {
        TTracedGuard<TMutex> guard(mutex);
        Y_DO_NOT_OPTIMIZE_AWAY(i);
    }
}
Y_CPU_BENCHMARK(FileTracedMutex, iface) {
    TTraceRegistryPtr logger = new TUsageLogger(MakeHolder<TOFStream>("bench.trace"));
    TGlobalRegistryGuard global(logger);

    TMutex mutex;

    for (size_t i : xrange(iface.Iterations())) {
        TTracedGuard<TMutex> guard(mutex);
        Y_DO_NOT_OPTIMIZE_AWAY(i);
    }
}
Y_CPU_BENCHMARK(ZipFileTracedMutex, iface) {
    TTraceRegistryPtr logger = new TUsageLogger("bench.trace");
    TGlobalRegistryGuard global(logger);

    TMutex mutex;

    for (size_t i : xrange(iface.Iterations())) {
        TTracedGuard<TMutex> guard(mutex);
        Y_DO_NOT_OPTIMIZE_AWAY(i);
    }
}

// User defined message
Y_CPU_BENCHMARK(NullTracedUserDefinedMessage, iface) {
    float testFloatValue = 0.123;
    double testDoubleValue = 0.321;
    const TStringBuf factorNames[] = {
        TStringBuf("FirstFactor"),
        TStringBuf("SecondFactor")};
    float factors[] = {
        0.1,
        0.2};

    for (size_t i : xrange(iface.Iterations())) {
        ITraceRegistry::TReportConstructor()
            .AddTag(TStringBuf("SomeTag"))
            .AddParam(TStringBuf("StringParam"), TStringBuf("StringValue"))
            .AddParam(TStringBuf("UintParam"), (ui64)123)
            .AddParam(TStringBuf("IntParam"), (i64)-123)
            .AddParam(TStringBuf("FloatParam"), testFloatValue)
            .AddParam(TStringBuf("DoubleParam"), testDoubleValue)
            .AddFactors(factorNames, factors);

        Y_DO_NOT_OPTIMIZE_AWAY(i);
    }
}
Y_CPU_BENCHMARK(TracedUserDefinedMessage, iface) {
    TTraceRegistryPtr logger = new TUsageLogger(MakeHolder<TBufferStream>());
    TGlobalRegistryGuard global(logger);

    float testFloatValue = 0.123;
    double testDoubleValue = 0.321;
    const TStringBuf factorNames[] = {
        TStringBuf("FirstFactor"),
        TStringBuf("SecondFactor")};
    float factors[] = {
        0.1,
        0.2};

    for (size_t i : xrange(iface.Iterations())) {
        ITraceRegistry::TReportConstructor()
            .AddTag(TStringBuf("SomeTag"))
            .AddParam(TStringBuf("StringParam"), TStringBuf("StringValue"))
            .AddParam(TStringBuf("UintParam"), (ui64)123)
            .AddParam(TStringBuf("IntParam"), (i64)-123)
            .AddParam(TStringBuf("FloatParam"), testFloatValue)
            .AddParam(TStringBuf("DoubleParam"), testDoubleValue)
            .AddFactors(factorNames, factors);

        Y_DO_NOT_OPTIMIZE_AWAY(i);
    }
}
Y_CPU_BENCHMARK(TracedUserDefinedMessageWithContext, iface) {
    TTraceRegistryPtr logger = new TUsageLogger(MakeHolder<TBufferStream>());
    TGlobalRegistryGuard global(logger);

    float testFloatValue = 0.123;
    double testDoubleValue = 0.321;
    const TStringBuf factorNames[] = {
        TStringBuf("FirstFactor"),
        TStringBuf("SecondFactor")};
    float factors[] = {
        0.1,
        0.2};

    auto context = TReportContext::New();
    for (size_t i : xrange(iface.Iterations())) {
        context.NewReport()
            .AddTag(TStringBuf("SomeTag"))
            .AddParam(TStringBuf("StringParam"), TStringBuf("StringValue"))
            .AddParam(TStringBuf("UintParam"), (ui64)123)
            .AddParam(TStringBuf("IntParam"), (i64)-123)
            .AddParam(TStringBuf("FloatParam"), testFloatValue)
            .AddParam(TStringBuf("DoubleParam"), testDoubleValue)
            .AddFactors(factorNames, factors);

        Y_DO_NOT_OPTIMIZE_AWAY(i);
    }
}
Y_CPU_BENCHMARK(TracedUserDefinedMessageWithChildContext, iface) {
    TTraceRegistryPtr logger = new TUsageLogger(MakeHolder<TBufferStream>());
    TGlobalRegistryGuard global(logger);

    float testFloatValue = 0.123;
    double testDoubleValue = 0.321;
    const TStringBuf factorNames[] = {
        TStringBuf("FirstFactor"),
        TStringBuf("SecondFactor")};
    float factors[] = {
        0.1,
        0.2};

    auto context = TReportContext::New();
    for (size_t i : xrange(iface.Iterations())) {
        context.NewChildContext().NewReport()
            .AddTag(TStringBuf("SomeTag"))
            .AddParam(TStringBuf("StringParam"), TStringBuf("StringValue"))
            .AddParam(TStringBuf("UintParam"), (ui64)123)
            .AddParam(TStringBuf("IntParam"), (i64)-123)
            .AddParam(TStringBuf("FloatParam"), testFloatValue)
            .AddParam(TStringBuf("DoubleParam"), testDoubleValue)
            .AddFactors(factorNames, factors);

        Y_DO_NOT_OPTIMIZE_AWAY(i);
    }
}
Y_CPU_BENCHMARK(FileTracedUserDefinedMessage, iface) {
    TTraceRegistryPtr logger = new TUsageLogger(MakeHolder<TOFStream>("bench.trace"));
    TGlobalRegistryGuard global(logger);

    float testFloatValue = 0.123;
    double testDoubleValue = 0.321;
    const TStringBuf factorNames[] = {
        TStringBuf("FirstFactor"),
        TStringBuf("SecondFactor")};
    float factors[] = {
        0.1,
        0.2};

    for (size_t i : xrange(iface.Iterations())) {
        ITraceRegistry::TReportConstructor()
            .AddTag(TStringBuf("SomeTag"))
            .AddParam(TStringBuf("StringParam"), TStringBuf("StringValue"))
            .AddParam(TStringBuf("UintParam"), (ui64)123)
            .AddParam(TStringBuf("IntParam"), (i64)-123)
            .AddParam(TStringBuf("FloatParam"), testFloatValue)
            .AddParam(TStringBuf("DoubleParam"), testDoubleValue)
            .AddFactors(factorNames, factors);

        Y_DO_NOT_OPTIMIZE_AWAY(i);
    }
}
Y_CPU_BENCHMARK(ZipFileTracedUserDefinedMessage, iface) {
    TTraceRegistryPtr logger = new TUsageLogger("bench.trace");
    TGlobalRegistryGuard global(logger);

    float testFloatValue = 0.123;
    double testDoubleValue = 0.321;
    const TStringBuf factorNames[] = {
        TStringBuf("FirstFactor"),
        TStringBuf("SecondFactor")};
    float factors[] = {
        0.1,
        0.2};

    for (size_t i : xrange(iface.Iterations())) {
        ITraceRegistry::TReportConstructor()
            .AddTag(TStringBuf("SomeTag"))
            .AddParam(TStringBuf("StringParam"), TStringBuf("StringValue"))
            .AddParam(TStringBuf("UintParam"), (ui64)123)
            .AddParam(TStringBuf("IntParam"), (i64)-123)
            .AddParam(TStringBuf("FloatParam"), testFloatValue)
            .AddParam(TStringBuf("DoubleParam"), testDoubleValue)
            .AddFactors(factorNames, factors);

        Y_DO_NOT_OPTIMIZE_AWAY(i);
    }
}
