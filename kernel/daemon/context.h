#pragma once

#include <kernel/daemon/config/daemon_config.h>

#include <util/system/types.h>
#include <util/datetime/base.h>

namespace NController {

struct TRestartServerStatistics {
    TDuration StopTime;
    TDuration StartTime;
};

template <typename TChildContext>
class TBaseContext {
public:
    TBaseContext(const ui32 rigidStopLevel)
        : RigidStopLevel(rigidStopLevel)
    {
    }
    template <typename TOtherChildContext>
    TBaseContext(const TBaseContext<TOtherChildContext>& other)
        : RigidStopLevel(other.GetRigidStopLevel())
        , Statistics(other.MutableStatistics())
        , SleepTimeout(other.GetSleepTimeout())
    {
    }
    ui32 GetRigidStopLevel() const {
        return RigidStopLevel;
    }
    TRestartServerStatistics* MutableStatistics() const {
        return Statistics;
    }
    TChildContext& SetStatistics(TRestartServerStatistics* const value) {
        Statistics = value;
        return *static_cast<TChildContext*>(this);
    }
    const TDuration& GetSleepTimeout() const {
        return SleepTimeout;
    }
    TChildContext& SetSleepTimeout(const TDuration& value) {
        SleepTimeout = value;
        return *static_cast<TChildContext*>(this);
    }
    TDuration* MutableStartTimePtr() const {
        if (Statistics) {
            return &Statistics->StartTime;
        } else {
            return nullptr;
        }
    }
    TDuration* MutableStopTimePtr() const {
        if (Statistics) {
            return &Statistics->StopTime;
        } else {
            return nullptr;
        }
    }
private:
    ui32 RigidStopLevel = 0;
    TRestartServerStatistics* Statistics = nullptr;
    TDuration SleepTimeout = TDuration::Zero();
};

template <typename TChildContext>
class TForceConfigReading {
public:
    TForceConfigReading() {
    }
    bool GetForceConfigsReading() const {
        return ForceConfigsReading;
    }
    TChildContext& SetForceConfigsReading(const bool value) {
        ForceConfigsReading = value;
        return *static_cast<TChildContext*>(this);
    }
private:
    bool ForceConfigsReading = false;
};

class TRestartContext : public TBaseContext<TRestartContext>, public TForceConfigReading<TRestartContext> {
public:
    TRestartContext(const ui32 rigidStopLevel)
        : TBaseContext(rigidStopLevel)
    {
    }
    template <typename TChildContext>
    TRestartContext(const TBaseContext<TChildContext>& ctx)
        : TBaseContext(ctx)
    {
    }
    bool GetReread() const {
        return Reread;
    }
    TRestartContext& SetReread(const bool value) {
        Reread = value;
        return *this;
    }
private:
    bool Reread = false;
};

class TDestroyServerContext : public TBaseContext<TDestroyServerContext> {
public:
    TDestroyServerContext(const ui32 rigidStopLevel)
        : TBaseContext(rigidStopLevel)
    {
    }
    template <typename TChildContext>
    TDestroyServerContext(const TBaseContext<TChildContext>& ctx)
        : TBaseContext(ctx)
    {
    }
};

}

