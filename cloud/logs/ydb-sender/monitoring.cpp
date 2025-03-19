#include "monitoring.h"

using namespace NMonitoring;

NMonitoring::TLabels mapToLabels(const THashMap<TString, TString>& labels) {
    NMonitoring::TLabels ret;
    for (const auto& label : labels) {
        ret.Add(label.first, label.second);
    }
    return ret;
}

TMonitoringCtx::TMonitoringCtx(const ui16 port, const THashMap<TString, TString>& labels)
    : _CommonLabels(mapToLabels(labels))
    , _Registry(new TMetricRegistry(_CommonLabels))
    , _MonSvc(port, "Cloud Logs YDB Sender", CreateFakeAuth())
{
    BatchRetriesC = _Registry->Counter({{"sensor", "batch_retry"}});
    BatchAcceptedC = _Registry->Counter({{"sensor", "batch_accepted"}});
    BatchMiniParsedC = _Registry->Counter({{"sensor", "minibatch_parsed"}});
    BatchMiniProcessedC = _Registry->Counter({{"sensor", "minibatch_processed"}});
    BatchMiniSavedC = _Registry->Counter({{"sensor", "minibatch_saved"}});
    YdbInFlightLimitG = _Registry->IntGauge({{"sensor", "ydb_in_flight_limit"}});
    YdbInFlightCurrentG = _Registry->IntGauge({{"sensor", "ydb_in_flight_current"}});
    BatchProcessedC = _Registry->Counter({{"sensor", "batch_processed"}});
    BatchCommittedC = _Registry->Counter({{"sensor", "batch_committed"}});

    _MonSvc.Register(new TVersionMonPage); // ver
    _MonSvc.Register(new TDiagMonPage);    // diag
    _MonSvc.Register(new TMetricRegistryPage("metrics", "Metrics", _Registry));

    _MonSvc.StartOrThrow();
}
