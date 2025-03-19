#include "volume_balancer_state.h"

#include <cloud/blockstore/libs/diagnostics/volume_stats.h>

#include <library/cpp/monlib/service/pages/templates.h>

#include <util/system/hostname.h>

namespace NCloud::NBlockStore::NStorage {

namespace {

////////////////////////////////////////////////////////////////////////////////

constexpr TDuration StateTolerance = TDuration::Seconds(60);
constexpr TDuration MaxPullDelay = TDuration::Hours(24);

}   // namespace

////////////////////////////////////////////////////////////////////////////////

TVolumeBalancerState::TVolumeBalancerState(TStorageConfigPtr storageConfig)
    : StorageConfig(std::move(storageConfig))
{}

void TVolumeBalancerState::UpdateVolumeStats(
    TVector<NProto::TVolumeBalancerDiskStats> stats,
    TPerfGuaranteesMap perfMap,
    ui64 sysPercents,
    ui64 userPercents,
    ui64 matBenchSys,
    ui64 matBenchUser,
    ui64 cpuLack,
    TInstant now)
{
    SysPercents = sysPercents;
    UserPercents = userPercents;
    MatBenchSys = matBenchSys;
    MatBenchUser = matBenchUser;
    CpuLack = cpuLack;

    bool emergencyCpu =
        cpuLack >= StorageConfig->GetCpuLackThreshold() ||
        MatBenchSys >= StorageConfig->GetCpuMatBenchNsSystemThreshold() ||
        MatBenchUser >= StorageConfig->GetCpuMatBenchNsUserThreshold();

    bool highLoad =
        sysPercents >= StorageConfig->GetPreemptionPushPercentage() ||
        userPercents >= StorageConfig->GetPreemptionPushPercentage() ||
        emergencyCpu;

    bool lowLoad =
        sysPercents <= StorageConfig->GetPreemptionPullPercentage() &&
        userPercents <= StorageConfig->GetPreemptionPullPercentage() &&
        !emergencyCpu;

    THashSet<TString> knownDisks;
    for (const auto& d: Volumes) {
        knownDisks.insert(d.first);
    }

    for (auto& v: stats) {
        auto it = Volumes.emplace(v.GetDiskId(), TVolumeInfo(StorageConfig->GetInitialPullDelay())).first;

        auto& info = it->second;

        if (v.GetIsLocal() || v.GetHost()) {
            info.LastSystemCpu = v.GetSystemCpu();
            info.LastUserCpu = v.GetUserCpu();
        }

        info.SysTime.Increment(info.LastSystemCpu, now);
        info.UserTime.Increment(info.LastUserCpu, now);
        info.PreemptionSource = v.GetPreemptionSource();
        info.CloudId = std::move(*v.MutableCloudId());
        info.Host = std::move(*v.MutableHost());

        if (v.GetIsLocal()) {
            info.NextPullAttempt = {};
        } else if (!info.NextPullAttempt) {
            info.NextPullAttempt = now + info.PullInterval;
            info.PullInterval = Min(MaxPullDelay, info.PullInterval * 2);
        }

        info.IsLocal = v.GetIsLocal();
        if (auto perfIt = perfMap.find(it->first); perfIt != perfMap.end()) {
            info.IsSuffering = perfIt->second;
        }
        knownDisks.erase(v.GetDiskId());
    }

    for (const auto& d: knownDisks) {
        Volumes.erase(d);
    }

    switch (State) {
        case EBalancerState::STATE_MEASURING: {
            if (highLoad) {
                OnStateChanged(EBalancerState::STATE_HI_LOAD, now);
                if (emergencyCpu) {
                    UpdateVolumeToPush();
                }
            } else if (lowLoad) {
                OnStateChanged(EBalancerState::STATE_LOW_LOAD, now);
            }
            break;
        }
        case EBalancerState::STATE_HI_LOAD: {
            if (highLoad) {
                if ((now - LastStateChange) >= StateTolerance || emergencyCpu) {
                    UpdateVolumeToPush();
                }
            } else if (lowLoad) {
                OnStateChanged(EBalancerState::STATE_LOW_LOAD, now);
            } else {
                OnStateChanged(EBalancerState::STATE_MEASURING, now);
            }
            break;
        }
        case EBalancerState::STATE_LOW_LOAD: {
            if (lowLoad) {
                if ((now - LastStateChange) >= StateTolerance) {
                    UpdateVolumeToPull(now);
                }
            } else if (highLoad) {
                OnStateChanged(EBalancerState::STATE_HI_LOAD, now);
            } else {
                OnStateChanged(EBalancerState::STATE_MEASURING, now);
            }
            break;
        }
        default:
            Y_FAIL("Unknown state %x", State);
            break;
    }
}

TString TVolumeBalancerState::GetVolumeToPush() const
{
    return VolumeToPush;
}

TString TVolumeBalancerState::GetVolumeToPull() const
{
    return VolumeToPull;
}

ui32 TVolumeBalancerState::GetState() const
{
    return static_cast<ui32>(State);
}

////////////////////////////////////////////////////////////////////////////////

void TVolumeBalancerState::RenderLocalVolumes(TStringStream& out) const
{
    HTML(out) {
        TABLE_SORTABLE_CLASS("table table-bordered") {
            TABLEHEAD() {
                TABLER() {
                    TABLEH() { out << "Volume"; }
                    TABLEH() { out << "User score"; }
                    TABLEH() { out << "System score"; }
                }
            }
            for (const auto& v: Volumes) {
                if (v.second.IsLocal) {
                    TABLER() {
                        TABLED() { out << v.first; }
                        TABLED() {
                            out << v.second.UserTime.GetValue();
                        }
                        TABLED() {
                            out << v.second.SysTime.GetValue();
                        }
                    }
                }
            }
        }
    }
}

void TVolumeBalancerState::RenderPreemptedVolumes(TStringStream& out) const
{
    HTML(out) {
        TABLE_SORTABLE_CLASS("table table-bordered") {
            TABLEHEAD() {
                TABLER() {
                    TABLEH() { out << "Volume"; }
                    TABLEH() { out << "User score"; }
                    TABLEH() { out << "System score"; }
                    TABLEH() { out << "Host"; }
                }
            }
            for (const auto& v: Volumes) {
                if (!v.second.IsLocal) {
                    TABLER() {
                        TABLED() { out << v.first; }
                        TABLED() {
                            out << v.second.UserTime.GetValue();
                        }
                        TABLED() {
                            out << v.second.SysTime.GetValue();
                        }
                        TABLED() {
                            out << v.second.Host;
                        }
                    }
                }
            }
        }
    }
}

void TVolumeBalancerState::RenderConfig(TStringStream& out) const
{
    HTML(out) {
        TABLE_SORTABLE_CLASS("table table-condensed") {
            TABLEHEAD() {
                TABLER() {
                    TABLED() { out << "Volume push percentage"; }
                    TABLED() { out << StorageConfig->GetPreemptionPushPercentage(); }
                }
                TABLER() {
                    TABLED() { out << "Volume pull percentage"; }
                    TABLED() { out << StorageConfig->GetPreemptionPullPercentage(); }
                }
                TABLER() {
                    TABLED() { out << "Volume preemption type"; }
                    TABLED() {
                        out << EVolumePreemptionType_Name(
                            StorageConfig->GetVolumePreemptionType());
                    }
                }
                TABLER() {
                    TABLED() { out << "CpuLackThreshold"; }
                    TABLED() { out << StorageConfig->GetCpuLackThreshold(); }
                }
                TABLER() {
                    TABLED() { out << "CpuMatBenchNs system pool threshold"; }
                    TABLED() { out << StorageConfig->GetCpuMatBenchNsSystemThreshold(); }
                }
                TABLER() {
                    TABLED() { out << "CpuMatBenchNs user pool threshold"; }
                    TABLED() { out << StorageConfig->GetCpuMatBenchNsUserThreshold(); }
                }
                TABLER() {
                    TABLED() { out << "Initial pull delay"; }
                    TABLED() { out << StorageConfig->GetInitialPullDelay(); }
                }
            }
        }
    }
}

void TVolumeBalancerState::RenderState(TStringStream& out) const
{
    HTML(out) {
        TABLE_SORTABLE_CLASS("table table-condensed") {
            TABLEHEAD() {
                TABLER() {
                    TABLED() { out << "Enabled"; }
                    TABLED() { out << ToString(IsEnabled); }
                }
                TABLER() {
                    TABLED() { out << "State"; }
                    TABLED() { out << ToString(State); }
                }
                TABLER() {
                    TABLED() { out << "System pool CPU load"; }
                    TABLED() { out << SysPercents << '%'; }
                }
                TABLER() {
                    TABLED() { out << "User pool CPU load"; }
                    TABLED() { out << UserPercents << '%'; }
                }
                TABLER() {
                    TABLED() { out << "CpuMatBenchSystem"; }
                    TABLED() { out << MatBenchSys; }
                }
                TABLER() {
                    TABLED() { out << "CpuMatBenchUser"; }
                    TABLED() { out << MatBenchUser; }
                }
                TABLER() {
                    TABLED() { out << "CPUs needed"; }
                    TABLED() { out << CpuLack; }
                }
            }
        }
    }
}

void TVolumeBalancerState::RenderHtml(TStringStream& out) const
{
    HTML(out) {
        H3() { out << "State"; }
        RenderState(out);

        H3() { out << "Config"; }
        RenderConfig(out);

        H3() { out << "Local Volumes"; }
        RenderLocalVolumes(out);

        H3() { out << "Preempted Volumes"; }
        RenderPreemptedVolumes(out);
    }
}

////////////////////////////////////////////////////////////////////////////////

void TVolumeBalancerState::UpdateVolumeToPush()
{
    if (State == EBalancerState::STATE_HI_LOAD) {
        const bool moveMostHeavy = StorageConfig->GetVolumePreemptionType() ==
            NProto::PREEMPTION_MOVE_MOST_HEAVY;

        ui64 value = moveMostHeavy ? 0 : Max<ui64>();
        for (auto v = Volumes.begin(); v != Volumes.end(); ++v) {
            const bool isEnabled = StorageConfig->IsBalancerFeatureEnabled(
                v->second.CloudId,
                {}  // TODO: FolderId
            );

            if (!v->second.IsLocal || !isEnabled) {
                continue;
            }

            auto cur = Max(
                v->second.SysTime.GetValue(),
                v->second.UserTime.GetValue());

            if (moveMostHeavy) {
                if (value <= cur) {
                    value = cur;
                    VolumeToPush = v->first;
                }
            } else {
                if (value >= cur) {
                    value = cur;
                    VolumeToPush = v->first;
                }
            }
        }
    }
}

void TVolumeBalancerState::UpdateVolumeToPull(TInstant now)
{
    if (State == EBalancerState::STATE_LOW_LOAD) {
        ui32 remoteCount = 0;
        for (const auto& v: Volumes) {
            if (!v.second.IsLocal &&
                (v.second.PreemptionSource == NProto::EPreemptionSource::SOURCE_BALANCER) &&
                v.second.NextPullAttempt <= now &&
                v.second.IsSuffering)
            {
                ++remoteCount;
            }
        }
        if (!remoteCount) {
            VolumeToPull = {};
            return;
        }
        ui32 itemToPeek = RandomNumber(remoteCount);
        for (const auto& v: Volumes) {
            if (!v.second.IsLocal &&
                (v.second.PreemptionSource == NProto::EPreemptionSource::SOURCE_BALANCER) &&
                (v.second.NextPullAttempt <= now))
            {
                if (!itemToPeek--) {
                    VolumeToPull = v.first;
                    break;
                }
            }
        }
    }
}

void TVolumeBalancerState::OnStateChanged(EBalancerState state, TInstant time)
{
    LastStateChange = time;
    State = state;
    switch (State) {
        case EBalancerState::STATE_HI_LOAD: {
            VolumeToPull = {};
            break;
        };
        case EBalancerState::STATE_LOW_LOAD: {
            VolumeToPush = {};
            break;
        };
        case EBalancerState::STATE_MEASURING: {
            VolumeToPull = {};
            VolumeToPush = {};
            break;
        }
        default:
            break;
    }
}

}   // namespace NCloud::NBlockStore::NStorage
