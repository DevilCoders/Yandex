#pragma once

#include "metrics.h"

#include <util/generic/hash.h>
#include <util/generic/set.h>
#include <util/system/mutex.h>

class TServiceMetric: public TCompositeMetric {
public:
    struct TResult {
        TMetricResult OkRequests;
        TMetricResult FailRequests;
        TMetricResult UserFailRequests;
        TMetricResult UnknownFailRequests;
        TMetricResult DeprecatedFailRequests;
        TMetricResult IncRequests;
        TMetricResult InFlightRequests;
        ui64 IncTimestamp;
        ui64 OkTimestamp;
    };
private:
    const TString ServiceName;

    TPart<TOrangeMetric> OkRequests;
    TPart<TOrangeMetric> FailRequests;
    TPart<TOrangeMetric> UserFailRequests;
    TPart<TOrangeMetric> UnknownFailRequests;
    TPart<TOrangeMetric> DeprecatedFailRequests;
    TPart<TOrangeMetric> IncRequests;
    TPart<TOrangeMetric> InFlightRequests;
    TPart<TOrangeHistogramMetrics> RequestTimings;
    TAtomic IncTimestamp;
    TAtomic OkTimestamp;
public:
    TServiceMetric(const TString& service, const TString& prefix = "");

    TResult CollectResult(TMetrics& host) const;

    void OnSuccess(TDuration processingTime, TInstant timestamp = TInstant::Zero());
    void OnFailure();
    void OnUserFailure();
    void OnDeprecatedFailure();
    void OnUnknownFailure();
    void OnIncoming(TInstant timestamp = TInstant::Zero());

    inline ui64 GetInFlightCount() const {
        return InFlightRequests.Get();
    }
};

typedef TAtomicSharedPtr<TServiceMetric> TServiceMetricPtr;
typedef THashMap<TString, TServiceMetricPtr> TMetricByService;

class TAutoMetricGuard {
public:
    TAutoMetricGuard(TServiceMetricPtr metric, TInstant start);
    TAutoMetricGuard(TServiceMetricPtr metric);
    ~TAutoMetricGuard();

private:
    const TServiceMetricPtr Metric;
    const TInstant Start;
};

class TStaticServiceMetrics {
public:
    TStaticServiceMetrics(const TString& prefix);
    TStaticServiceMetrics(TMetrics* host, const TString& prefix);
    virtual ~TStaticServiceMetrics();

    virtual TServiceMetricPtr Get(const TString& service) const;
    TServiceMetric::TResult GetResult(const TString& service) const;

    void Dump(IOutputStream& out);
    void AddStaticMetrics(const TSet<TString>& services);
    TServiceMetricPtr AddStaticMetric(const TString& service);

protected:
    void UnregisterMetrics();

protected:
    const TString Prefix;
    const THolder<TMetrics> OwnedHost;
    TMetrics* const Host;
    TMetricByService Metrics;

    TServiceMetricPtr UnknownService;
};

class TDynamicServiceMetrics: public TStaticServiceMetrics {
public:
    TDynamicServiceMetrics(const TString& prefix)
        : TStaticServiceMetrics(prefix)
    {}
    TDynamicServiceMetrics(TMetrics* host, const TString& prefix)
        : TStaticServiceMetrics(host, prefix)
    {}

    virtual TServiceMetricPtr Get(const TString& service) const override;

private:
    TMutex Lock;
};
