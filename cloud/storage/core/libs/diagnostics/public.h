#pragma once

#include <util/datetime/base.h>
#include <util/generic/ptr.h>
#include <util/generic/size_literals.h>
#include <util/generic/string.h>

#include <memory>
#include <utility>

class TLog;

namespace NMonitoring {
    class IMonPage;
    using IMonPagePtr = TIntrusivePtr<IMonPage>;

    struct TDynamicCounters;
    using TDynamicCountersPtr = TIntrusivePtr<TDynamicCounters>;

    class IMetricConsumer;
}

namespace NCloud {

////////////////////////////////////////////////////////////////////////////////

constexpr ui32 LargeRequestSize = 64_KB;
constexpr TDuration UpdateCountersInterval = TDuration::Seconds(15);
constexpr TDuration UpdateStatsInterval = TDuration::Seconds(1);
constexpr TDuration DumpTracksInterval = TDuration::Seconds(15);
constexpr ui64 DumpTracksLimit = 150;

////////////////////////////////////////////////////////////////////////////////

struct IAsyncLogger;
using IAsyncLoggerPtr = std::shared_ptr<IAsyncLogger>;

struct ILoggingService;
using ILoggingServicePtr = std::shared_ptr<ILoggingService>;

struct IMonitoringService;
using IMonitoringServicePtr = std::shared_ptr<IMonitoringService>;

class TLatencyHistogram;
class TSizeHistogram;

struct ITraceProcessor;
using ITraceProcessorPtr = std::shared_ptr<ITraceProcessor>;

struct ITraceReader;
using ITraceReaderPtr = std::shared_ptr<ITraceReader>;

struct ITraceSerializer;
using ITraceSerializerPtr = std::shared_ptr<ITraceSerializer>;

using TBucketInfo = std::pair<ui32, ui64>;
using TPercentileDesc = std::pair<double, TString>;

using TDiagnosticsRequestType = ui32;

class TRequestCounters;
using TRequestCountersPtr = std::shared_ptr<TRequestCounters>;

struct TIncompleteRequest;

struct IIncompleteRequestProvider;
using IIncompleteRequestProviderPtr = std::shared_ptr<IIncompleteRequestProvider>;

struct IStats;
using IStatsPtr = std::shared_ptr<IStats>;

struct IStatsUpdater;
using IStatsUpdaterPtr = std::shared_ptr<IStatsUpdater>;

}   // namespace NCloud
