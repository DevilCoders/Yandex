#pragma once

#include "accessor.h"

#include <util/datetime/base.h>
#include <util/thread/pool.h>
#include <util/system/event.h>
#include <library/cpp/logger/global/global.h>
#include <kernel/common_server/library/logging/events.h>

template <class... TArgs>
class IStartStopProcessImpl {
private:
    TAtomic ActiveFlag = 0;
protected:
    virtual bool DoStart(TArgs... args) = 0;
    virtual bool DoStop() = 0;
public:
    virtual ~IStartStopProcessImpl() {
        CHECK_WITH_LOG(AtomicGet(ActiveFlag) == 0) << "Process is active and cannot be destroyed" << Endl;
    }

    bool IsActive() const {
        return AtomicGet(ActiveFlag);
    }

    virtual bool Start(TArgs... args) noexcept {
        CHECK_WITH_LOG(AtomicCas(&ActiveFlag, 1, 0));
        try {
            return DoStart(args...);
        } catch (...) {
            TFLEventLog::Log("problem on start")("reason", CurrentExceptionMessage());
            return false;
        }
    }

    virtual bool Stop() noexcept {
        CHECK_WITH_LOG(AtomicCas(&ActiveFlag, 0, 1));
        try {
            if (!DoStop()) {
                return false;
            }
        } catch (...) {
            TFLEventLog::Log("problem on stop")("reason", CurrentExceptionMessage());
            return false;
        }
        return true;
    }
};

using IStartStopProcess = IStartStopProcessImpl<>;

class IAutoActualization: public IStartStopProcess {
    RTLINE_ACCEPTOR_DEF(IAutoActualization, Name, TString);
    RTLINE_ACCEPTOR(IAutoActualization, Period, TDuration, TDuration::Seconds(1));
    RTLINE_ACCEPTOR(IAutoActualization, MetricPeriod, TDuration, TDuration::Seconds(1));
private:
    class TRefreshAgent;
    class TMetricAgent;

private:
    TThreadPool Queue;
    TManualEvent RefreshAgentStopEvent;
    TManualEvent MetricAgentStopEvent;

protected:
    virtual bool DoStart() override;
    virtual bool DoStop() override;
    virtual void DoWaitPeriod(const TInstant start);
    virtual bool GetStartFailIsProblem() const {
        return true;
    }
public:
    virtual bool MetricSignal() {
        return true;
    }

    virtual bool Refresh() = 0;

    ~IAutoActualization() = default;
    IAutoActualization(const TString& name)
        : Name(name)
    {
    }

    IAutoActualization(const TString& name, const TDuration period)
        : Name(name)
        , Period(period)
    {
    }
};
