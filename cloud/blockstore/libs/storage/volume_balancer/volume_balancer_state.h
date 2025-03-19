#pragma once

#include "public.h"

#include <cloud/blockstore/libs/storage/api/service.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/features_config.h>

#include <ydb/core/tablet/tablet_metrics.h>

#include <util/datetime/base.h>
#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

enum class EBalancerState
{
    STATE_MEASURING,
    STATE_HI_LOAD,
    STATE_LOW_LOAD,
};

////////////////////////////////////////////////////////////////////////////////

class TVolumeBalancerState
{
    using TAverageHolder =
        NKikimr::NMetrics::TDecayingAverageValue<
            ui64,
            NKikimr::NMetrics::DurationPer15Seconds,
            NKikimr::NMetrics::DurationPerSecond>;

    struct TVolumeInfo
    {
        TString CloudId;
        TAverageHolder SysTime;
        TAverageHolder UserTime;

        bool IsLocal = true;
        NProto::EPreemptionSource PreemptionSource = NProto::EPreemptionSource::SOURCE_BALANCER;

        TString Host;

        ui64 LastUserCpu = 0;
        ui64 LastSystemCpu = 0;

        TInstant NextPullAttempt;
        TDuration PullInterval;

        bool IsSuffering = false;

        TVolumeInfo(TDuration pullInterval)
            : PullInterval(pullInterval)
        {}
    };

public:
    using TPerfGuaranteesMap = THashMap<TString, bool>;

private:
    TStorageConfigPtr StorageConfig;

    ui64 SysPercents = 0;
    ui64 UserPercents = 0;
    ui64 MatBenchSys = 0;
    ui64 MatBenchUser = 0;
    ui64 CpuLack = 0;

    THashMap<TString, TVolumeInfo> Volumes;

    EBalancerState State = EBalancerState::STATE_MEASURING;
    TInstant LastStateChange;

    TString VolumeToPush;
    TString VolumeToPull;

    bool IsEnabled = true;

public:
    TVolumeBalancerState(TStorageConfigPtr storageConfig);

    TString GetVolumeToPush() const;
    TString GetVolumeToPull() const;

    void UpdateVolumeStats(
        TVector<NProto::TVolumeBalancerDiskStats> stats,
        TPerfGuaranteesMap perfMap,
        ui64 sysPercents,
        ui64 userPercents,
        ui64 matBenchSys,
        ui64 matBenchUser,
        ui64 cpuLack,
        TInstant now);

    ui32 GetState() const;

    void RenderHtml(TStringStream& out) const;

    void SetEnabled(bool enable)
    {
        IsEnabled = enable;
    }

    bool GetEnabled() const
    {
        return IsEnabled;
    }

private:
    void RenderLocalVolumes(TStringStream& out) const;
    void RenderPreemptedVolumes(TStringStream& out) const;
    void RenderConfig(TStringStream& out) const;
    void RenderState(TStringStream& out) const;

    void UpdateVolumeToPush();
    void UpdateVolumeToPull(TInstant now);

    void OnStateChanged(EBalancerState state, TInstant time);
};

}   // namespace NCloud::NBlockStore::NStorage
