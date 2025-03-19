#pragma once

#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/disk_counters.h>
#include <cloud/blockstore/libs/storage/core/metrics.h>
#include <cloud/blockstore/libs/storage/core/request_info.h>

#include <library/cpp/actors/core/actorid.h>

#include <util/datetime/base.h>
#include <util/generic/hash.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

struct TDiskPerfData
{
    bool CountersRegistered = false;
    bool HasCheckpoint = false;
    bool HasClients = false;
    bool IsPreempted = false;

    TPartitionDiskCounters DiskCounters;
    TVolumeSelfCounters VolumeSelfCounters;

    TPartitionDiskCounters YdbDiskCounters;
    TVolumeSelfCounters YdbVolumeSelfCounters;

    NMonitoring::TDynamicCounters::TCounterPtr VolumeBindingCounter;

    ui64 VolumeSystemCpu = 0;
    ui64 VolumeUserCpu = 0;

    TDiskPerfData(EPublishingPolicy policy)
        : DiskCounters(policy)
        , VolumeSelfCounters(policy)
        , YdbDiskCounters(policy)
        , YdbVolumeSelfCounters(policy)
    {}
};

////////////////////////////////////////////////////////////////////////////////

struct TTotalCounters
{
    TPartitionDiskCounters PartAcc;
    TVolumeSelfCounters VolumeAcc;
    TSimpleCounter TotalDiskCount;
    TSimpleCounter TotalPartitionCount;

    TTotalCounters(EPublishingPolicy policy)
        : PartAcc(policy)
        , VolumeAcc(policy)
    {};

    void Register(NMonitoring::TDynamicCountersPtr counters);
    void Reset();
    void Publish(TInstant now);
    void UpdatePartCounters(const TPartitionDiskCounters& source);
    void UpdateVolumeSelfCounters(const TVolumeSelfCounters& source);
};

////////////////////////////////////////////////////////////////////////////////

struct TVolumeRequestCounters
{
    TCumulativeCounter ReadCount;
    TCumulativeCounter ReadBytes;
    TCumulativeCounter WriteCount;
    TCumulativeCounter WriteBytes;
    TCumulativeCounter ZeroCount;
    TCumulativeCounter ZeroBytes;

    void Register(NMonitoring::TDynamicCountersPtr counters);
    void Publish(TInstant now);
    void Reset();
    void UpdateCounters(const TPartitionDiskCounters& source);
};

////////////////////////////////////////////////////////////////////////////////

class TBlobLoadCounters
{
public:
    TBlobLoadCounters(
        const TString& mediaKind,
        ui64 maxGroupReadIops,
        ui64 maxGroupWriteIops,
        ui64 maxGroupReadThroughput,
        ui64 maxGroupWriteThroughput);

    void Register(NMonitoring::TDynamicCountersPtr counters);
    void Publish(const NBlobMetrics::TBlobLoadMetrics& metrics, TInstant now);

public:
    const TString MediaKind;
    const ui64 MaxGroupReadIops;
    const ui64 MaxGroupWriteIops;
    const ui64 MaxGroupReadThroughput;
    const ui64 MaxGroupWriteThroughput;

    TSolomonValueHolder UsedGroupsCount;
};

////////////////////////////////////////////////////////////////////////////////

struct TVolumeStatsInfo
{
    TMaybe<NProto::TVolume> VolumeInfo;

    TDiskPerfData PerfCounters;
    NBlobMetrics::TBlobLoadMetrics OffsetBlobMetrics;

    TVolumeStatsInfo()
        : PerfCounters(EPublishingPolicy::All)
    {}

    bool IsNonReplicated() const
    {
        if (!VolumeInfo) {
            return false;
        }

        switch (VolumeInfo->GetStorageMediaKind()) {
            case NProto::STORAGE_MEDIA_SSD_NONREPLICATED:
            case NProto::STORAGE_MEDIA_SSD_LOCAL:
            case NProto::STORAGE_MEDIA_SSD_MIRROR2:
            case NProto::STORAGE_MEDIA_SSD_MIRROR3:
                return true;
            default:
                return false;
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

class TStatsServiceState
{
private:
    using TVolumesMap = THashMap<TString, TVolumeStatsInfo>;

    TVolumesMap VolumesById;

    TTotalCounters Total;
    TTotalCounters Hdd;
    TTotalCounters Ssd;
    TTotalCounters SsdNonrepl;
    TTotalCounters SsdMirror2;
    TTotalCounters SsdMirror3;
    TTotalCounters SsdLocal;
    TTotalCounters SsdSystem;

    TVolumeRequestCounters LocalVolumes;
    TVolumeRequestCounters NonlocalVolumes;

    TBlobLoadCounters SsdBlobLoadCounters;
    TBlobLoadCounters HddBlobLoadCounters;

    bool StatsUploadingCompleted = true;

public:
    void RemoveVolume(const TString& diskId);
    TVolumeStatsInfo* GetVolume(const TString& diskId);
    TVolumeStatsInfo* GetOrAddVolume(const TString& diskId);

    TStatsServiceState(const TStorageConfig& config)
        : Total(EPublishingPolicy::All)
        , Hdd(EPublishingPolicy::Repl)
        , Ssd(EPublishingPolicy::Repl)
        , SsdNonrepl(EPublishingPolicy::NonRepl)
        , SsdMirror2(EPublishingPolicy::NonRepl)
        , SsdMirror3(EPublishingPolicy::NonRepl)
        , SsdLocal(EPublishingPolicy::NonRepl)
        , SsdSystem(EPublishingPolicy::Repl)
        , SsdBlobLoadCounters(
            config.GetCommonSSDPoolKind(),
            config.GetMaxSSDGroupReadIops(),
            config.GetMaxSSDGroupWriteIops(),
            config.GetMaxSSDGroupReadBandwidth(),
            config.GetMaxSSDGroupWriteBandwidth())
        , HddBlobLoadCounters(
            config.GetCommonHDDPoolKind(),
            config.GetMaxHDDGroupReadIops(),
            config.GetMaxHDDGroupWriteIops(),
            config.GetMaxHDDGroupReadBandwidth(),
            config.GetMaxHDDGroupWriteBandwidth())
    {};

    const TVolumesMap& GetVolumes() const
    {
        return VolumesById;
    }

    TVolumesMap& GetVolumes()
    {
        return VolumesById;
    }

    TTotalCounters& GetTotalCounters()
    {
        return Total;
    }

    TTotalCounters& GetHddCounters()
    {
        return Hdd;
    }

    TTotalCounters& GetSsdCounters()
    {
        return Ssd;
    }

    TTotalCounters& GetSsdNonreplCounters()
    {
        return SsdNonrepl;
    }

    TTotalCounters& GetSsdMirror2Counters()
    {
        return SsdMirror2;
    }

    TTotalCounters& GetSsdMirror3Counters()
    {
        return SsdMirror3;
    }

    TTotalCounters& GetSsdLocalCounters()
    {
        return SsdLocal;
    }

    TTotalCounters& GetSsdSystemCounters()
    {
        return SsdSystem;
    }

    TTotalCounters& GetCounters(const NProto::TVolume& volume)
    {
        auto mediaKind = volume.GetStorageMediaKind();
        switch (mediaKind) {
            case NCloud::NProto::STORAGE_MEDIA_SSD: {
                return volume.GetIsSystem() ? SsdSystem : Ssd;
            }
            case NCloud::NProto::STORAGE_MEDIA_SSD_NONREPLICATED: return SsdNonrepl;
            case NCloud::NProto::STORAGE_MEDIA_SSD_MIRROR2: return SsdMirror2;
            case NCloud::NProto::STORAGE_MEDIA_SSD_MIRROR3: return SsdMirror3;
            case NCloud::NProto::STORAGE_MEDIA_SSD_LOCAL: return SsdLocal;
            case NCloud::NProto::STORAGE_MEDIA_HDD:
            case NCloud::NProto::STORAGE_MEDIA_HYBRID:
            case NCloud::NProto::STORAGE_MEDIA_DEFAULT: return Hdd;
            default: {}
        }

        Y_FAIL("unsupported media kind: %u", static_cast<ui32>(mediaKind));
    }

    TVolumeRequestCounters& GetLocalVolumesCounters()
    {
        return LocalVolumes;
    }

    TVolumeRequestCounters& GetNonlocalVolumesCounters()
    {
        return NonlocalVolumes;
    }

    TBlobLoadCounters& GetSsdBlobCounters()
    {
        return SsdBlobLoadCounters;
    }

    TBlobLoadCounters& GetHddBlobCounters()
    {
        return HddBlobLoadCounters;
    }

    void SetStatsUploadingCompleted(bool completed)
    {
        StatsUploadingCompleted = completed;
    }

    bool GetStatsUploadingCompleted() const
    {
        return StatsUploadingCompleted;
    }
};

}   // namespace NCloud::NBlockStore::NStorage
