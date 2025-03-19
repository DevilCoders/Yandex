#pragma once

#include "public.h"

#include <cloud/storage/core/libs/common/error.h>

#include <util/datetime/base.h>
#include <util/generic/flags.h>

namespace NCloud {

////////////////////////////////////////////////////////////////////////////////

class TRequestCounters
{
    struct TStatCounters;

public:
    enum class EOption
    {
        ReportHistogram =       (1 << 0),
        OnlyReadWriteRequests = (1 << 1),
    };

    using TRequestType = TDiagnosticsRequestType;

    Y_DECLARE_FLAGS(EOptions, EOption);

private:
    const std::function<TString(TRequestType)> RequestType2Name;
    const std::function<bool(TRequestType)> IsReadWriteRequestType;
    const EOptions Options;

    TVector<TStatCounters> CountersByRequest;
    TVector<TRequestCountersPtr> Subscribers;

public:
    TRequestCounters(
        ITimerPtr timer,
        ui32 requestCount,
        std::function<TString(TRequestType)> requestType2Name,
        std::function<bool(TRequestType)> isReadWriteRequestType,
        EOptions options = {});
    ~TRequestCounters();

    void Register(NMonitoring::TDynamicCounters& counters);

    void Subscribe(TRequestCountersPtr subscriber);

    ui64 RequestStarted(
        TRequestType requestType,
        ui32 requestBytes);

    TDuration RequestCompleted(
        TRequestType requestType,
        ui64 requestStarted,
        TDuration postponedTime,
        ui32 requestBytes,
        EErrorKind errorKind);

    void AddRetryStats(TRequestType requestType, EErrorKind errorKind);

    void RequestPostponed(TRequestType requestType);
    void RequestAdvanced(TRequestType requestType);
    void RequestFastPathHit(TRequestType requestType);

    void AddIncompleteStats(
        TRequestType requestType,
        TDuration requestTime);

    void UpdateStats(bool updatePercentiles = false);

private:
    void RequestStartedImpl(
        TRequestType requestType,
        ui32 requestBytes);

    void RequestCompletedImpl(
        TRequestType requestType,
        TDuration requestTime,
        TDuration postponedTime,
        ui32 requestBytes,
        EErrorKind errorKind);

    bool ShouldReport(TRequestType requestType) const;

    template<typename TMethod, typename... TArgs>
    void NotifySubscribers(TMethod&& m, TArgs&&... args);
};

Y_DECLARE_OPERATORS_FOR_FLAGS(TRequestCounters::EOptions);

}   // namespace NCloud
