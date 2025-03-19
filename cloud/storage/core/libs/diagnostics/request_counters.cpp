#include "request_counters.h"

#include "max_calculator.h"
#include "weighted_percentile.h"

#include <cloud/storage/core/libs/common/timer.h>
#include <cloud/storage/core/libs/common/verify.h>

#include <library/cpp/monlib/dynamic_counters/counters.h>

#include <util/datetime/cputimer.h>
#include <util/generic/algorithm.h>
#include <util/generic/vector.h>
#include <util/string/builder.h>

#include <utility>

namespace NCloud {

using namespace NMonitoring;

namespace {

////////////////////////////////////////////////////////////////////////////////

template <typename TDerived>
struct THistBase
{
    struct TBucket
    {
        const ui32 Value = 0;
        TDynamicCounters::TCounterPtr Counter;
        TBucket(ui32 value)
            : Value(value)
            , Counter(new TCounterForPtr(true))
        {}
    };

    void Register(
        TDynamicCounters& counters,
        TCountableBase::EVisibility vis = TCountableBase::EVisibility::Public)
    {
        auto* pThis = static_cast<TDerived*>(this);

        for (auto& bucket: pThis->Buckets) {
            bucket.Counter = counters.GetCounter(pThis->GetName(bucket.Value), true, vis);
        }
    }

    void Increment(ui32 value)
    {
        auto* pThis = static_cast<TDerived*>(this);

        auto comparer = [] (const TBucket& bucket, ui32 value) {
            return bucket.Value < value;
        };

        auto it = LowerBound(
            pThis->Buckets.begin(),
            pThis->Buckets.end(),
            value,
            comparer);
        STORAGE_VERIFY(
            it != pThis->Buckets.end(),
            "Bucket",
            value);

        it->Counter->Inc();
    }

    TVector<TBucketInfo> GetBuckets() const
    {
        const auto* pThis = static_cast<const TDerived*>(this);

        TVector<TBucketInfo> result(Reserve(pThis->Buckets.size()));
        for (const auto& bucket: pThis->Buckets) {
            result.emplace_back(
                bucket.Value,
                bucket.Counter->Val());
        }

        return result;
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TTimeHist
    : public THistBase<TTimeHist>
{
    static constexpr size_t BUCKETS_COUNT = 15;
    std::array<TBucket, BUCKETS_COUNT> Buckets = {{
        1, 2, 5, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 35000,
        Max<ui32>()
    }};

    static TString GetName(ui32 value)
    {
        if (value == Max<ui32>()) {
            return "Inf";
        } else {
            return TStringBuilder() << value << "ms";
        }
    }

    void Increment(TDuration requestTime)
    {
        THistBase::Increment(requestTime.MilliSeconds());
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TSizeHist
    : public THistBase<TSizeHist>
{
    static constexpr size_t BUCKETS_COUNT = 12;
    std::array<TBucket, BUCKETS_COUNT> Buckets = {{
        4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096,
        Max<ui32>()
    }};

    static TString GetName(ui32 value)
    {
        if (value == Max<ui32>()) {
            return "Inf";
        } else {
            return TStringBuilder() << value << "KB";
        }
    }

    void Increment(ui32 requestBytes)
    {
        THistBase::Increment(requestBytes / 1024);
    }
};

////////////////////////////////////////////////////////////////////////////////

class TRequestPercentiles
{
    using TDynamicCounterPtr = TDynamicCounters::TCounterPtr;

private:
    TVector<TDynamicCounterPtr> Counters;

    TVector<ui64> Prev;

public:
    void Register(TDynamicCounters& counters)
    {
        const auto& percentiles = GetDefaultPercentiles();
        for (ui32 i = 0; i < percentiles.size(); ++i) {
            Counters.emplace_back(
                counters.GetCounter(percentiles[i].second, false));
        }
    }

    void Update(const TVector<TBucketInfo>& update)
    {
        if (Prev.size() < update.size()) {
            Prev.resize(update.size());
        }

        TVector<TBucketInfo> delta(Reserve(update.size()));
        for (ui32 i = 0; i < update.size(); ++i) {
            delta.emplace_back(
                update[i].first,
                update[i].second - Prev[i]);
            Prev[i] = update[i].second;
        }

        auto result = CalculateWeightedPercentiles(
            delta,
            GetDefaultPercentiles());

        for (ui32 i = 0; i < Min(Counters.size(), result.size()); ++i) {
            *Counters[i] = result[i];
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

TIntrusivePtr<TDynamicCounters> MakeVisibilitySubgroup(
    TDynamicCounters& cunters,
    const TString& name,
    const TString& value,
    TCountableBase::EVisibility visibility)
{
    TIntrusivePtr<TDynamicCounters> subgroup;

    if (subgroup = cunters.FindSubgroup(name, value)) {
        if (subgroup->Visibility() != visibility) {
            subgroup = MakeIntrusive<TDynamicCounters>(visibility);
            cunters.ReplaceSubgroup(name, value, subgroup);
        }
    } else {
        subgroup = MakeIntrusive<TDynamicCounters>(visibility);
        cunters.RegisterSubgroup(name, value, subgroup);
    }
    return subgroup;
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

struct TRequestCounters::TStatCounters
{
    template<typename Type>
    struct TFailedAndSuccess
    {
        Type Success;
        Type Failed;
    };

    bool IsReadWriteRequest = false;
    bool ReportHistogram = false;

    TDynamicCounters::TCounterPtr Count;
    TDynamicCounters::TCounterPtr MaxCount;
    TDynamicCounters::TCounterPtr Time;
    TDynamicCounters::TCounterPtr MaxTime;
    TDynamicCounters::TCounterPtr MaxSize;
    TDynamicCounters::TCounterPtr RequestBytes;
    TDynamicCounters::TCounterPtr MaxRequestBytes;
    TDynamicCounters::TCounterPtr InProgress;
    TDynamicCounters::TCounterPtr MaxInProgress;
    TDynamicCounters::TCounterPtr InProgressBytes;
    TDynamicCounters::TCounterPtr MaxInProgressBytes;
    TDynamicCounters::TCounterPtr PostponedQueueSize;
    TDynamicCounters::TCounterPtr MaxPostponedQueueSize;
    TDynamicCounters::TCounterPtr PostponedCount;
    TDynamicCounters::TCounterPtr FastPathHits;

    TDynamicCounters::TCounterPtr Errors;
    TDynamicCounters::TCounterPtr ErrorsFatal;
    TDynamicCounters::TCounterPtr ErrorsRetriable;
    TDynamicCounters::TCounterPtr ErrorsThrottling;
    TDynamicCounters::TCounterPtr ErrorsSession;
    TDynamicCounters::TCounterPtr ErrorsSilent;
    TDynamicCounters::TCounterPtr Retries;

    TSizeHist SizeHist;
    TRequestPercentiles SizePercentiles;

    TTimeHist TimeHist;
    TTimeHist TimeHistSmall;
    TTimeHist TimeHistLarge;
    TTimeHist ExecutionTimeHist;
    TTimeHist ExecutionTimeHistSmall;
    TTimeHist ExecutionTimeHistLarge;
    TTimeHist PostponedTimeHist;
    TRequestPercentiles TimePercentiles;
    TRequestPercentiles ExecutionTimePercentiles;

    TMaxCalculator<DEFAULT_BUCKET_COUNT> MaxTimeCalc;
    TMaxCalculator<DEFAULT_BUCKET_COUNT> MaxSizeCalc;
    TMaxCalculator<DEFAULT_BUCKET_COUNT> MaxInProgressCalc;
    TMaxCalculator<DEFAULT_BUCKET_COUNT> MaxInProgressBytesCalc;
    TMaxCalculator<DEFAULT_BUCKET_COUNT> MaxPostponedQueueSizeCalc;
    TMaxPerSecondCalculator<DEFAULT_BUCKET_COUNT> MaxCountCalc;
    TMaxPerSecondCalculator<DEFAULT_BUCKET_COUNT> MaxRequestBytesCalc;

    explicit TStatCounters(ITimerPtr timer)
        : MaxTimeCalc(timer)
        , MaxSizeCalc(timer)
        , MaxInProgressCalc(timer)
        , MaxInProgressBytesCalc(timer)
        , MaxPostponedQueueSizeCalc(timer)
        , MaxCountCalc{timer}
        , MaxRequestBytesCalc{timer}
    {}

    TStatCounters(const TStatCounters&) = delete;
    TStatCounters(TStatCounters&&) = default;

    TStatCounters& operator = (const TStatCounters&) = delete;
    TStatCounters& operator = (TStatCounters&&) = default;

    void Init(
        TDynamicCounters& counters,
        bool isReadWriteRequest,
        bool reportHistogram)
    {
        IsReadWriteRequest = isReadWriteRequest;
        ReportHistogram = reportHistogram;

        Count = counters.GetCounter("Count", true);

        Time = counters.GetCounter("Time", true);
        MaxTime = counters.GetCounter("MaxTime");

        InProgress = counters.GetCounter("InProgress");
        MaxInProgress = counters.GetCounter("MaxInProgress");

        Errors = counters.GetCounter("Errors", true);
        ErrorsFatal = counters.GetCounter("Errors/Fatal", true);
        ErrorsRetriable = counters.GetCounter("Errors/Retriable", true);
        ErrorsThrottling = counters.GetCounter("Errors/Throttling", true);
        ErrorsSession = counters.GetCounter("Errors/Session", true);
        Retries = counters.GetCounter("Retries", true);

        const auto visibleHistogram = ReportHistogram
            ? TCountableBase::EVisibility::Public
            : TCountableBase::EVisibility::Private;

        TimeHist.Register(*MakeVisibilitySubgroup(
            counters,
            "histogram",
            "Time",
            visibleHistogram), visibleHistogram);

        if (!ReportHistogram) {
            TimePercentiles.Register(*counters.GetSubgroup("percentiles", "Time"));
        }

        if (IsReadWriteRequest) {
            ErrorsSilent = counters.GetCounter("Errors/Silent", true);

            MaxSize = counters.GetCounter("MaxSize");
            MaxCount = counters.GetCounter("MaxCount");

            RequestBytes = counters.GetCounter("RequestBytes", true);
            MaxRequestBytes = counters.GetCounter("MaxRequestBytes");

            InProgressBytes = counters.GetCounter("InProgressBytes");
            MaxInProgressBytes = counters.GetCounter("MaxInProgressBytes");

            if (ReportHistogram) {
                auto smallClassGroup = counters.GetSubgroup("sizeclass", "Small");
                auto largeClassGroup = counters.GetSubgroup("sizeclass", "Large");

                SizeHist.Register(*counters.GetSubgroup("histogram", "Size"));
                TimeHistSmall.Register(*smallClassGroup->GetSubgroup("histogram", "Time"));
                TimeHistLarge.Register(*largeClassGroup->GetSubgroup("histogram", "Time"));
                ExecutionTimeHist.Register(
                    *counters.GetSubgroup("histogram", "ExecutionTime"));
                ExecutionTimeHistSmall.Register(
                    *smallClassGroup->GetSubgroup("histogram", "ExecutionTime"));
                ExecutionTimeHistLarge.Register(
                    *largeClassGroup->GetSubgroup("histogram", "ExecutionTime"));
            } else {
                SizePercentiles.Register(*counters.GetSubgroup("percentiles", "Size"));
                ExecutionTimePercentiles.Register(
                    *counters.GetSubgroup("percentiles", "ExecutionTime"));
            }

            PostponedTimeHist.Register(*MakeVisibilitySubgroup(
                counters,
                "histogram",
                "ThrottlerDelay",
                visibleHistogram), visibleHistogram);

            PostponedQueueSize = counters.GetCounter("PostponedQueueSize");
            MaxPostponedQueueSize = counters.GetCounter("MaxPostponedQueueSize");

            PostponedCount = counters.GetCounter("PostponedCount", true);
            FastPathHits = counters.GetCounter("FastPathHits", true);
        }
    }

    void Started(ui32 requestBytes)
    {
        MaxInProgressCalc.Add(InProgress->Inc());

        if (IsReadWriteRequest) {
            MaxInProgressBytesCalc.Add(InProgressBytes->Add(requestBytes));
        }
    }

    void Completed(ui32 requestBytes)
    {
        InProgress->Dec();

        if (IsReadWriteRequest) {
            InProgressBytes->Sub(requestBytes);
        }
    }

    void AddStats(
        TDuration requestTime,
        TDuration postponedTime,
        ui32 requestBytes,
        EErrorKind errorKind)
    {
        const bool failed = ( errorKind != EErrorKind::Success ) &&
            ( errorKind != EErrorKind::ErrorSilent || !IsReadWriteRequest );

        if (failed) {
            Errors->Inc();
        } else {
            Count->Inc();
        }

        switch (errorKind) {
            case EErrorKind::Success:
                break;
            case EErrorKind::ErrorFatal:
                ErrorsFatal->Inc();
                break;
            case EErrorKind::ErrorRetriable:
                ErrorsRetriable->Inc();
                break;
            case EErrorKind::ErrorThrottling:
                ErrorsThrottling->Inc();
                break;
            case EErrorKind::ErrorSession:
                ErrorsSession->Inc();
                break;
            case EErrorKind::ErrorSilent:
                if (IsReadWriteRequest) {
                    ErrorsSilent->Inc();
                }
                break;
        }

        auto execTime = requestTime - postponedTime;
        Time->Add(requestTime.MicroSeconds());
        TimeHist.Increment(requestTime);
        MaxTimeCalc.Add(execTime.MicroSeconds());

        if (IsReadWriteRequest) {
            MaxCountCalc.Add(1);
            RequestBytes->Add(requestBytes);
            MaxRequestBytesCalc.Add(requestBytes);

            SizeHist.Increment(requestBytes);
            MaxSizeCalc.Add(requestBytes);

            if (requestBytes < LargeRequestSize) {
                TimeHistSmall.Increment(requestTime);
                ExecutionTimeHistSmall.Increment(execTime);
            } else {
                TimeHistLarge.Increment(requestTime);
                ExecutionTimeHistLarge.Increment(execTime);
            }

            ExecutionTimeHist.Increment(execTime);
            PostponedTimeHist.Increment(postponedTime);
        }
    }

    void AddRetryStats(EErrorKind errorKind)
    {
        switch (errorKind) {
            case EErrorKind::ErrorRetriable:
                ErrorsRetriable->Inc();
                break;
            case EErrorKind::ErrorThrottling:
                ErrorsThrottling->Inc();
                break;
            case EErrorKind::ErrorSession:
                ErrorsSession->Inc();
                break;
            case EErrorKind::Success:
            case EErrorKind::ErrorFatal:
            case EErrorKind::ErrorSilent:
                Y_VERIFY_DEBUG(false);
                return;
        }

        Errors->Inc();
        Retries->Inc();
    }

    void RequestPostponed()
    {
        if (IsReadWriteRequest) {
            PostponedCount->Inc();
            MaxPostponedQueueSizeCalc.Add(PostponedQueueSize->Inc());
        }
    }

    void RequestFastPathHit()
    {
        if (IsReadWriteRequest) {
            FastPathHits->Inc();
        }
    }

    void RequestAdvanced()
    {
        if (IsReadWriteRequest) {
            PostponedQueueSize->Dec();
        }
    }

    void AddIncompleteStats(TDuration requestTime)
    {
        MaxTimeCalc.Add(requestTime.MicroSeconds());
    }

    void UpdateStats(bool updatePercentiles)
    {
        *MaxInProgress = MaxInProgressCalc.NextValue();
        *MaxTime = MaxTimeCalc.NextValue();

        if (IsReadWriteRequest) {
            *MaxCount = MaxCountCalc.NextValue();
            *MaxSize = MaxSizeCalc.NextValue();
            *MaxRequestBytes = MaxRequestBytesCalc.NextValue();
            *MaxInProgressBytes = MaxInProgressBytesCalc.NextValue();
            *MaxPostponedQueueSize = MaxPostponedQueueSizeCalc.NextValue();
            if (updatePercentiles && !ReportHistogram) {
                SizePercentiles.Update(SizeHist.GetBuckets());
                TimePercentiles.Update(TimeHist.GetBuckets());
                ExecutionTimePercentiles.Update(ExecutionTimeHist.GetBuckets());
            }
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

TRequestCounters::TRequestCounters(
        ITimerPtr timer,
        ui32 requestCount,
        std::function<TString(TRequestType)> requestType2Name,
        std::function<bool(TRequestType)> isReadWriteRequestType,
        EOptions options)
    : RequestType2Name(std::move(requestType2Name))
    , IsReadWriteRequestType(std::move(isReadWriteRequestType))
    , Options(options)
{
    CountersByRequest.reserve(requestCount);
    for (ui32 i = 0; i < requestCount; ++i) {
        CountersByRequest.emplace_back(timer);
    }
}

TRequestCounters::~TRequestCounters()
{}

void TRequestCounters::Register(TDynamicCounters& counters)
{
    for (TRequestType t = 0; t < CountersByRequest.size(); ++t) {
        if (ShouldReport(t)) {
            auto requestGroup = counters.GetSubgroup(
                "request",
                RequestType2Name(t));

            bool reportHistogram = (Options & EOption::ReportHistogram);
            CountersByRequest[t].Init(
                *requestGroup,
                IsReadWriteRequestType(t),
                reportHistogram);
        }
    }
}

void TRequestCounters::Subscribe(TRequestCountersPtr subscriber)
{
    Subscribers.push_back(subscriber);
}

ui64 TRequestCounters::RequestStarted(
    TRequestType requestType,
    ui32 requestBytes)
{
    RequestStartedImpl(requestType, requestBytes);
    return GetCycleCount();
}

TDuration TRequestCounters::RequestCompleted(
    TRequestType requestType,
    ui64 requestStarted,
    TDuration postponedTime,
    ui32 requestBytes,
    EErrorKind errorKind)
{
    auto requestTime = CyclesToDurationSafe(GetCycleCount() - requestStarted);
    RequestCompletedImpl(requestType, requestTime, postponedTime, requestBytes, errorKind);
    return requestTime;
}

void TRequestCounters::AddRetryStats(
    TRequestType requestType,
    EErrorKind errorKind)
{
    if (ShouldReport(requestType)) {
        CountersByRequest[requestType].AddRetryStats(errorKind);
    }
    NotifySubscribers(
        &TRequestCounters::AddRetryStats,
        requestType,
        errorKind);
}

void TRequestCounters::RequestPostponed(TRequestType requestType)
{
    if (ShouldReport(requestType)) {
        CountersByRequest[requestType].RequestPostponed();
    }
    NotifySubscribers(
        &TRequestCounters::RequestPostponed,
        requestType);
}

void TRequestCounters::RequestFastPathHit(TRequestType requestType)
{
    if (ShouldReport(requestType)) {
        CountersByRequest[requestType].RequestFastPathHit();
    }
    NotifySubscribers(
        &TRequestCounters::RequestFastPathHit,
        requestType);
}

void TRequestCounters::RequestAdvanced(TRequestType requestType)
{
    if (ShouldReport(requestType)) {
        CountersByRequest[requestType].RequestAdvanced();
    }
    NotifySubscribers(
        &TRequestCounters::RequestAdvanced,
        requestType);
}

void TRequestCounters::AddIncompleteStats(
    TRequestType requestType,
    TDuration requestTime)
{
    if (ShouldReport(requestType)) {
        CountersByRequest[requestType].AddIncompleteStats(requestTime);
    }
    NotifySubscribers(
        &TRequestCounters::AddIncompleteStats,
        requestType,
        requestTime);
}

void TRequestCounters::UpdateStats(bool updatePercentiles)
{
    for (size_t t = 0; t < CountersByRequest.size(); ++t) {
        if (ShouldReport(t)) {
            CountersByRequest[t].UpdateStats(updatePercentiles);
        }
    }
    // NOTE subscribers are updated by their owners
}

void TRequestCounters::RequestStartedImpl(
    TRequestType requestType,
    ui32 requestBytes)
{
    if (ShouldReport(requestType)) {
        CountersByRequest[requestType].Started(requestBytes);
    }
    NotifySubscribers(
        &TRequestCounters::RequestStartedImpl,
        requestType,
        requestBytes);
}

void TRequestCounters::RequestCompletedImpl(
    TRequestType requestType,
    TDuration requestTime,
    TDuration postponedTime,
    ui32 requestBytes,
    EErrorKind errorKind)
{
    if (ShouldReport(requestType)) {
        CountersByRequest[requestType].Completed(requestBytes);
        CountersByRequest[requestType].AddStats(requestTime, postponedTime, requestBytes, errorKind);
    }
    NotifySubscribers(
        &TRequestCounters::RequestCompletedImpl,
        requestType,
        requestTime,
        postponedTime,
        requestBytes,
        errorKind);
}

bool TRequestCounters::ShouldReport(TRequestType requestType) const
{
    return requestType < CountersByRequest.size() && (IsReadWriteRequestType(requestType)
        || !(Options & EOption::OnlyReadWriteRequests));
}

template<typename TMethod, typename... TArgs>
void TRequestCounters::NotifySubscribers(TMethod&& m, TArgs&&... args)
{
    for (auto& s: Subscribers) {
        (s.get()->*m)(std::forward<TArgs>(args)...);
    }
}

}   // namespace NCloud
