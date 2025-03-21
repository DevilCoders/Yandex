#pragma once

#include "public.h"

#include <cloud/storage/core/libs/common/startable.h>
#include <cloud/storage/core/libs/diagnostics/monitoring.h>

#include <library/cpp/containers/ring_buffer/ring_buffer.h>
#include <library/cpp/lwtrace/log.h>
#include <library/cpp/monlib/service/pages/html_mon_page.h>
#include <library/cpp/monlib/service/pages/index_mon_page.h>
#include <library/cpp/monlib/service/pages/templates.h>

#include <util/datetime/base.h>
#include <util/generic/vector.h>
#include <util/system/thread.h>

namespace NLWTrace {
    class TManager;
    class TQuery;
}   // namespace NLWTrace

namespace NCloud {

////////////////////////////////////////////////////////////////////////////////

struct TEntry
{
    TInstant Ts;
    ui64 Date = 0;
    NLWTrace::TTrackLog TrackLog;
    TString Tag;
};


struct ITraceReader
{
    const TString Id;
    virtual void ForEachTraceLog(std::function<void (const TEntry&)> fn) = 0;
    virtual void Push(TThread::TId, const NLWTrace::TTrackLog&) = 0;
    virtual void Reset() = 0;

    virtual ~ITraceReader() = default;

    ITraceReader(const TString id)
        : Id(id)
    {
    }

};

////////////////////////////////////////////////////////////////////////////////

struct ITraceProcessor
    : public IStartable
{
    virtual ~ITraceProcessor() = default;
};

////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////

ITraceProcessorPtr CreateTraceProcessor(
    ITimerPtr timer,
    ISchedulerPtr scheduler,
    ILoggingServicePtr logging,
    IMonitoringServicePtr monitoring,
    TString componentName,
    NLWTrace::TManager& lwManager,
    TVector<ITraceReaderPtr> readers);

ITraceProcessorPtr CreateTraceProcessorStub();

ITraceReaderPtr CreateTraceLogger(
    TString id,
    ILoggingServicePtr logging,
    TString componentName);

ITraceReaderPtr CreateSlowRequestsFilter(
    TString id,
    ILoggingServicePtr logging,
    TString componentName,
    TDuration hddThreshold,
    TDuration ssdThreshold,
    TDuration nonReplicatedSSDThreshold);

NLWTrace::TQuery ProbabilisticQuery(
    const TVector<std::tuple<TString, TString>>& probes,
    ui32 samplingRate);

NLWTrace::TQuery ProbabilisticQuery(
    const TVector<std::tuple<TString, TString>>& probes,
    ui32 samplingRate,
    ui32 shuttleCount);

bool ReaderIdMatch(const TString& traceType, const TString& readerId);

}   // namespace NCloud
