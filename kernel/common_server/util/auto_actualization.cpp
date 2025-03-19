#include "auto_actualization.h"

#include <library/cpp/logger/global/global.h>
#include <kernel/common_server/library/unistat/cache.h>

class IAutoActualization::TRefreshAgent: public IObjectInQueue {
private:
    IAutoActualization* Owner = nullptr;

public:
    TRefreshAgent(IAutoActualization* owner)
        : Owner(owner)
    {
        CHECK_WITH_LOG(Owner);
    }

    void Process(void* /*threadSpecificResource*/) override {
        while (Owner->IsActive()) {
            DEBUG_LOG << "Start refresh for " << Owner->GetName() << Endl;
            const TInstant start = Now();
            try {
                if (!Owner->Refresh()) {
                    TCSSignals::Signal("auto_actualization")("process", Owner->GetName())("code", "failed_refresh");
                } else {
                    TCSSignals::Signal("auto_actualization")("process", Owner->GetName())("code", "success");
                }
            } catch (...) {
                TCSSignals::Signal("auto_actualization")("process", Owner->GetName())("code", "exception");
                ERROR_LOG << "Cannot refresh " << Owner->GetName() << ": " << CurrentExceptionMessage() << Endl;
            }
            Owner->DoWaitPeriod(start);
        }
    }
};

class IAutoActualization::TMetricAgent: public IObjectInQueue {
private:
    IAutoActualization* Owner = nullptr;
public:
    TMetricAgent(IAutoActualization* owner)
        : Owner(owner)
    {
        CHECK_WITH_LOG(Owner);
    }

    void Process(void* /*threadSpecificResource*/) override {
        while (Owner->IsActive()) {
            const TInstant start = Now();
            try {
                if (!Owner->MetricSignal()) {
                    WARNING_LOG << "Cannot refresh metric " << Owner->GetName() << Endl;
                }
            } catch (...) {
                ERROR_LOG << "Cannot refresh metric " << Owner->GetName() << ": " << CurrentExceptionMessage() << Endl;
            }
            const TInstant finish = Now();
            if (finish - start < Owner->GetMetricPeriod()) {
                Owner->MetricAgentStopEvent.WaitT(Owner->GetMetricPeriod() - (finish - start));
            }
        }
    }
};

bool IAutoActualization::DoStart() {
    CHECK_WITH_LOG(Refresh() || !GetStartFailIsProblem());
    Queue.Start(2);
    Queue.SafeAddAndOwn(MakeHolder<TRefreshAgent>(this));
    Queue.SafeAddAndOwn(MakeHolder<TMetricAgent>(this));
    return true;
}

bool IAutoActualization::DoStop() {
    RefreshAgentStopEvent.Signal();
    MetricAgentStopEvent.Signal();
    Queue.Stop();
    return true;
}

void IAutoActualization::DoWaitPeriod(const TInstant start) {
    const TInstant finish = Now();
    if (finish - start < GetPeriod()) {
        RefreshAgentStopEvent.WaitT(GetPeriod() - (finish - start));
    }
}
