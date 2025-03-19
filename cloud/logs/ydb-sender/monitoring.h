#pragma once

#include <library/cpp/monlib/metrics/metric.h>
#include <library/cpp/monlib/metrics/metric_registry.h>
#include <library/cpp/monlib/encode/prometheus/prometheus.h>

#include <library/cpp/monlib/service/monservice.h>
#include <library/cpp/monlib/service/pages/pre_mon_page.h>
#include <library/cpp/monlib/service/pages/version_mon_page.h>
#include <library/cpp/monlib/service/pages/diag_mon_page.h>
#include <library/cpp/monlib/service/pages/index_mon_page.h>
#include <library/cpp/monlib/service/pages/registry_mon_page.h>
#include <library/cpp/monlib/service/pages/resource_mon_page.h>

class TMonitoringCtx {
    using TCounter = NMonitoring::TCounter;
    using TGauge = NMonitoring::TIntGauge;

public:
    explicit TMonitoringCtx(ui16 port, const THashMap<TString, TString>& labels);

private:
    NMonitoring::TLabels _CommonLabels;
    TAtomicSharedPtr<NMonitoring::TMetricRegistry> _Registry;
    NMonitoring::TMonService2 _MonSvc;

public:
    // TLogPipeline sensors
    TCounter* BatchRetriesC;
    TCounter* BatchAcceptedC;
    TCounter* BatchMiniParsedC;
    TCounter* BatchMiniProcessedC;
    TCounter* BatchMiniSavedC;
    TGauge* YdbInFlightLimitG;
    TGauge* YdbInFlightCurrentG;
    TCounter* BatchProcessedC;
    TCounter* BatchCommittedC;
};

static TAtomicSharedPtr<TMonitoringCtx> Monitoring;
