#include "limited_source.h"
#include <library/cpp/json/json_writer.h>

namespace NCommonProxy {

    TLimitedSource::TReplier::TReplier(const TLimitedSource& owner)
        : TSource::TReplier(owner)
        , Owner(owner)
    {}

    TLimitedSource::TReplier::~TReplier() {
    }

    TLimitedSource::TLimitedSource(const TString& name, const TProcessorsConfigs& configs)
        : TSource(name, configs)
        , Config(*configs.Get<TConfig>(name))
    {}

    void TLimitedSource::DoStop() {
        Stopped = true;
    }

    void TLimitedSource::DoWait() {
        Finished.Wait();
    }

     void TLimitedSource::DoProcess(TDataSet::TPtr input, IReplier::TPtr replier) const {
         TSource::DoProcess(input, replier);
         if (ProcessedCounter.GetCount() % Config.CheckAlertsPeriod == 0) {
             CheckAlerts(true);
         }
    }

    void TLimitedSource::CheckAlerts(bool periodic) const {
        for (const auto& alert : Config.Alerts) {
            ui64 processed;
            ui64 failed;
            if (alert.Period == TDuration::Zero()) {
                if (periodic)
                    continue;
                processed = ProcessedCounter.GetCount();
                failed = FailedCounter.GetCount();
            } else {
                if (!periodic)
                    continue;
                processed = ProcessedCounter.GetCount(alert.Period);
                failed = FailedCounter.GetCount(alert.Period);
            }
            double succses = processed > 1e-6 ? Max<double>(1. - ((double)failed) / processed, 0.) : 1.;
            if (succses < alert.SuccessRatio) {
                Report();
                TString period = alert.Period.ToString();
                FATAL_LOG << "alert failed: period = " << alert.Period.ToString() << "; must_success = " << alert.SuccessRatio << "; success = " << succses << Endl;
                _exit(EXIT_FAILURE);
            }
        }
    }

    void TLimitedSource::Report() const {
        NJson::TJsonValue report;
        CollectInfo(report);
        Cout << NJson::WriteJson(report, true, true) << Endl;
    }

    void TLimitedSource::Run() {
        Stopped = false;
        DoRun();
        while (AtomicGet(InProgress))
            Sleep(TDuration::MilliSeconds(100));
        CheckAlerts(false);
        Report();
        Finished.Signal();
    }

}
