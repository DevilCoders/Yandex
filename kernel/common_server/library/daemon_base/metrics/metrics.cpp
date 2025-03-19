#include "metrics.h"
#include "messages.h"

#include <library/cpp/logger/global/global.h>

#include <util/generic/algorithm.h>
#include <util/stream/file.h>
#include <util/stream/tokenizer.h>
#include <util/system/progname.h>

namespace {
    TString METRICS_PREFIX;
    size_t MAX_AGE_TO_KEEP_DAYS = 10;  // days

    class TAutoStartMertics: public TMetricsController {
    public:
        TAutoStartMertics() {
            Start();
        }
    };
}

void TCompositeMetric::Register(TMetrics& metrics) const {
    for (auto&& m : Metrics) {
        m->RegisterMetric(metrics);
    }
}

void TCompositeMetric::Deregister(TMetrics& metrics) const {
    for (auto&& m : Metrics) {
        m->UnregisterMetric(metrics);
    }
}

void TCompositeMetric::AddMetric(const IMetric* metric) {
    VERIFY_WITH_LOG(Find(Metrics.begin(), Metrics.end(), metric) == Metrics.end(), "part of a composite metric is registered twice");
    Metrics.push_back(metric);
}

void TCompositeMetric::RemoveMetric(const IMetric* metric) {
    auto p = Find(Metrics.begin(), Metrics.end(), metric);
    VERIFY_WITH_LOG(p != Metrics.end(), "trying to remove non-registered part of a composite metric");
    Metrics.erase(p);
}

const TString& GetMetricsPrefix() {
    if (!METRICS_PREFIX) {
        METRICS_PREFIX = GetProgramName() + "_";
        METRICS_PREFIX.to_upper(0, 1);
    }
    return METRICS_PREFIX;
}

void SetMetricsPrefix(const TString& prefix) {
    if (prefix.size()) {
        METRICS_PREFIX = prefix;
        METRICS_PREFIX.to_upper(0, 1);
    }
}

size_t GetMetricsMaxAgeDays() {
    return MAX_AGE_TO_KEEP_DAYS;
}

void SetMetricsMaxAgeDays(size_t age) {
    if (age) {
        MAX_AGE_TO_KEEP_DAYS = age;
    }
}

TMetrics& GetGlobalMetrics() {
    TMetrics& result = *Singleton<TAutoStartMertics>();
    Y_ASSERT(result.Running());
    return result;
}

void CollectMetrics(IOutputStream& out) {
    TCollectMetricsMessage collector(out);
    SendGlobalMessage(collector);
    TMetricsProcessor::UpdateMetricValues(GetGlobalMetrics());
    TMetricsProcessor::OutputMetrics(GetGlobalMetrics(), out);
}

TMaybe<TMetricResult> GetMetricResult(const TString& name, const TMetrics& host) {
    TVector<TMetricResult> result;
    host.GetMetric(name.data(), result);
    if (result.empty()) {
        return Nothing();
    }

    return result.front();
}

TMaybe<TMetricResult> GetMetricResult(const TOrangeMetric& metric, const TMetrics& host) {
    return GetMetricResult(metric.GetName(), host);
}
