#include "servicemetrics.h"

#include <library/cpp/logger/global/global.h>

namespace {
    const size_t Digits = 4;
    const TVector<size_t> Bounds = { 10, 20, 50, 100, 200, 500, 1000, 2000, 5000 };
}

TServiceMetric::TServiceMetric(const TString& service, const TString& prefix /*= ""*/)
    : ServiceName(service)
    , OkRequests(this, prefix + service + "_ok")
    , FailRequests(this, prefix + service + "_server_fails")
    , UserFailRequests(this, prefix + service + "_user_fails")
    , UnknownFailRequests(this, prefix + service + "_unknown_fails")
    , DeprecatedFailRequests(this, prefix + service + "_deprecated_fails")
    , IncRequests(this, prefix + service + "_incoming")
    , InFlightRequests(this, prefix + service + "_in_flight")
    , RequestTimings(this, prefix + service + "_timings", Digits, Bounds)
    , IncTimestamp(0)
    , OkTimestamp(0)
{}

void TServiceMetric::OnSuccess(TDuration processingTime, TInstant timestamp) {
    OkRequests.Inc();
    InFlightRequests.Dec();
    RequestTimings.Hit(processingTime.MilliSeconds());
    AtomicSet(OkTimestamp, timestamp.GetValue());
}

void TServiceMetric::OnFailure() {
    FailRequests.Inc();
    InFlightRequests.Dec();
}

void TServiceMetric::OnDeprecatedFailure() {
    DeprecatedFailRequests.Inc();
    InFlightRequests.Dec();
}

void TServiceMetric::OnUserFailure() {
    UserFailRequests.Inc();
    InFlightRequests.Dec();
}

void TServiceMetric::OnUnknownFailure() {
    UnknownFailRequests.Inc();
    InFlightRequests.Dec();
}

void TServiceMetric::OnIncoming(TInstant timestamp) {
    IncRequests.Inc();
    InFlightRequests.Inc();
    AtomicSet(IncTimestamp, timestamp.GetValue());
}

TServiceMetric::TResult TServiceMetric::CollectResult(TMetrics& host) const {
    TServiceMetric::TResult result;
    result.FailRequests = *GetMetricResult(FailRequests, host);
    result.OkRequests   = *GetMetricResult(OkRequests, host);
    result.IncRequests  = *GetMetricResult(IncRequests, host);
    result.InFlightRequests = *GetMetricResult(InFlightRequests, host);
    result.UserFailRequests = *GetMetricResult(UserFailRequests, host);
    result.UnknownFailRequests = *GetMetricResult(UnknownFailRequests, host);
    result.DeprecatedFailRequests = *GetMetricResult(DeprecatedFailRequests, host);
    result.IncTimestamp = IncTimestamp;
    result.OkTimestamp  = OkTimestamp;
    return result;
}

TStaticServiceMetrics::TStaticServiceMetrics(const TString& prefix /*= ""*/)
    : Prefix(prefix)
    , OwnedHost(new TMetricsController)
    , Host(OwnedHost.Get())
    , UnknownService(new TServiceMetric("unknown"))
{
    Host->Start();
}

TStaticServiceMetrics::TStaticServiceMetrics(TMetrics* host, const TString& prefix /*= ""*/)
    : Prefix(prefix)
    , Host(host)
    , UnknownService(new TServiceMetric("unknown"))
{}

TStaticServiceMetrics::~TStaticServiceMetrics() {
    for (auto&& i : Metrics) {
        Y_VERIFY_DEBUG(!i.second->GetInFlightCount(), "%lu", i.second->GetInFlightCount());
    }
    UnregisterMetrics();
}

TServiceMetricPtr TDynamicServiceMetrics::Get(const TString& service) const {
    TGuard<TMutex> guard(Lock);
    TMetricByService::const_iterator p = Metrics.find(service);
    if (p == Metrics.end()) {
        const_cast<TDynamicServiceMetrics*>(this)->AddStaticMetric(service);
        p = Metrics.find(service);
    }
    return p->second;
}

void TStaticServiceMetrics::Dump(IOutputStream& out) {
    VERIFY_WITH_LOG(Host, "incorrect metrics host");
    TMetricsProcessor::OutputMetrics(*Host, out);
}

TServiceMetricPtr TStaticServiceMetrics::AddStaticMetric(const TString& service) {
    VERIFY_WITH_LOG(Host, "incorrect metrics host");
    auto metrics = new TServiceMetric(service, Prefix);
    metrics->Register(*Host);
    DEBUG_LOG << "registering metrics for service " << service << Endl;

    auto result = Metrics.insert(std::make_pair(service, metrics));
    VERIFY_WITH_LOG(result.second, "trying to insert metrics for the second time %s", service.data());
    return result.first->second;
}

TServiceMetricPtr TStaticServiceMetrics::Get(const TString& service) const {
    TMetricByService::const_iterator p = Metrics.find(service);
    return p != Metrics.end() ? p->second : UnknownService;
}

TServiceMetric::TResult TStaticServiceMetrics::GetResult(const TString& service) const {
    TServiceMetricPtr metric = Get(service);
    return metric->CollectResult(*Host);
}

void TStaticServiceMetrics::AddStaticMetrics(const TSet<TString>& services) {
    for (auto&& service : services) {
        AddStaticMetric(service);
    }
}

void TStaticServiceMetrics::UnregisterMetrics() {
    VERIFY_WITH_LOG(Host, "incorrect metrics host");
    for (auto&& metric : Metrics) {
        metric.second->Deregister(*Host);
    }
}

TAutoMetricGuard::TAutoMetricGuard(TServiceMetricPtr metric, TInstant start)
    : Metric(metric)
    , Start(start)
{
    if (Metric) {
        Metric->OnIncoming();
    }
}

TAutoMetricGuard::TAutoMetricGuard(TServiceMetricPtr metric)
    : TAutoMetricGuard(metric, Now())
{
}

TAutoMetricGuard::~TAutoMetricGuard() {
    if (!Metric) {
        return;
    }

    if (UncaughtException()) {
        Metric->OnFailure();
    } else {
        const TDuration duration = Now() - Start;
        Metric->OnSuccess(duration);
    }
}
