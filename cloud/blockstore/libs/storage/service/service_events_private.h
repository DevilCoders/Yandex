#pragma once

#include "public.h"

#include <cloud/blockstore/libs/kikimr/components.h>
#include <cloud/blockstore/libs/kikimr/events.h>
#include <cloud/blockstore/libs/storage/core/disk_counters.h>
#include <cloud/blockstore/libs/storage/core/metrics.h>
#include <cloud/blockstore/libs/storage/core/request_info.h>
#include <cloud/blockstore/libs/storage/protos/part.pb.h>
#include <cloud/blockstore/libs/storage/protos/volume.pb.h>

#include <ydb/core/tablet/tablet_counters.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

struct TEvServicePrivate
{
    //
    // VolumeTabletStatus notification
    //

    struct TVolumeTabletStatus
    {
        const ui64 TabletId = 0;
        const NProto::TVolume VolumeInfo;
        const NActors::TActorId VolumeActor;
        const NProto::TError Error;

        TVolumeTabletStatus(
                ui64 tabletId,
                const NProto::TVolume& volumeInfo,
                const NActors::TActorId& volumeActor)
            : TabletId(tabletId)
            , VolumeInfo(volumeInfo)
            , VolumeActor(volumeActor)
        {}

        TVolumeTabletStatus(const NProto::TError& error)
            : Error(error)
        {}
    };

    //
    // StartVolumeActorStopped notification
    //

    struct TStartVolumeActorStopped
    {
        const NProto::TError Error;

        TStartVolumeActorStopped(const NProto::TError& error)
            : Error(error)
        {}
    };

    //
    // InactiveClientsTimeout notification
    //

    struct TInactiveClientsTimeout
    {
    };

    //
    // StartVolume request
    //

    struct TStartVolumeRequest
    {
        const ui64 TabletId = 0;

        TStartVolumeRequest(ui64 tabletId)
            : TabletId(tabletId)
        {}
    };

    //
    // StartVolume response
    //

    struct TStartVolumeResponse
    {
        const NProto::TVolume VolumeInfo;

        TStartVolumeResponse() = default;

        TStartVolumeResponse(NProto::TVolume volumeInfo)
            : VolumeInfo(std::move(volumeInfo))
        {}
    };

    //
    // StopVolume request
    //

    struct TStopVolumeRequest
    {
    };

    //
    // StopVolume response
    //

    struct TStopVolumeResponse
    {
    };

    //
    // MountRequestProcessed notification
    //

    struct TMountRequestProcessed
    {
        const NProto::TVolume Volume;
        const ui64 MountStartTick;
        const NProto::TMountVolumeRequest Request;

        const TRequestInfoPtr RequestInfo;
        const ui64 VolumeTabletId;

        const bool HadLocalStart;
        const NProto::EVolumeBinding BindingType;
        const NProto::EPreemptionSource PreemptionSource;

        TMountRequestProcessed(
                NProto::TVolume volume,
                NProto::TMountVolumeRequest request,
                ui64 mountStartTick,
                TRequestInfoPtr requestInfo,
                ui64 volumeTabletId,
                bool hadLocalStart,
                NProto::EVolumeBinding bindingType,
                NProto::EPreemptionSource preemptionSource)
            : Volume(std::move(volume))
            , MountStartTick(mountStartTick)
            , Request(std::move(request))
            , RequestInfo(std::move(requestInfo))
            , VolumeTabletId(volumeTabletId)
            , HadLocalStart(hadLocalStart)
            , BindingType(bindingType)
            , PreemptionSource(preemptionSource)
        {}
    };

    //
    // UnmountRequestProcessed notification
    //

    struct TUnmountRequestProcessed
    {
        const TString DiskId;
        const TString ClientId;
        const NActors::TActorId RequestSender;
        const NProto::EControlRequestSource Source;

        TUnmountRequestProcessed(
                TString diskId,
                TString clientId,
                const NActors::TActorId& requestSender,
                NProto::EControlRequestSource source)
            : DiskId(std::move(diskId))
            , ClientId(std::move(clientId))
            , RequestSender(requestSender)
            , Source(source)
        {}
    };

    //
    // UploadDisksStatsCompleted notification
    //

    struct TSessionActorDied
    {
        TString DiskId;
    };

    //
    // VolumePipeReset notification
    //

    struct TVolumePipeReset
    {
        ui64 ResetTick = 0;

        explicit TVolumePipeReset(ui64 tick)
            : ResetTick(tick)
        {}
    };

    //
    // ResetPipeClient notification
    //

    struct TResetPipeClient
    {
    };

    //
    // Events declaration
    //

    enum EEvents
    {
        EvBegin = TBlockStorePrivateEvents::SERVICE_START,

        EvVolumeTabletStatus,
        EvStartVolumeActorStopped,
        EvInactiveClientsTimeout,
        EvStartVolumeRequest,
        EvStartVolumeResponse,
        EvStopVolumeRequest,
        EvStopVolumeResponse,
        EvMountRequestProcessed,
        EvUnmountRequestProcessed,
        EvSessionActorDied,
        EvVolumePipeReset,
        EvResetPipeClient,

        EvEnd
    };

    static_assert(EvEnd < (int)TBlockStorePrivateEvents::SERVICE_END,
        "EvEnd expected to be < TBlockStorePrivateEvents::SERVICE_END");


    using TEvVolumeTabletStatus = TRequestEvent<
        TVolumeTabletStatus,
        EvVolumeTabletStatus
    >;

    using TEvStartVolumeActorStopped = TRequestEvent<
        TStartVolumeActorStopped,
        EvStartVolumeActorStopped
    >;

    using TEvInactiveClientsTimeout = TRequestEvent<
        TInactiveClientsTimeout,
        EvInactiveClientsTimeout
    >;

    using TEvStartVolumeRequest = TRequestEvent<
        TStartVolumeRequest,
        EvStartVolumeRequest
    >;

    using TEvStartVolumeResponse = TResponseEvent<
        TStartVolumeResponse,
        EvStartVolumeResponse
    >;

    using TEvStopVolumeRequest = TRequestEvent<
        TStopVolumeRequest,
        EvStopVolumeRequest
    >;

    using TEvStopVolumeResponse = TResponseEvent<
        TStopVolumeResponse,
        EvStopVolumeResponse
    >;

    using TEvMountRequestProcessed = TResponseEvent<
        TMountRequestProcessed,
        EvMountRequestProcessed
    >;

    using TEvUnmountRequestProcessed = TResponseEvent<
        TUnmountRequestProcessed,
        EvUnmountRequestProcessed
    >;

    using TEvSessionActorDied = TResponseEvent<
        TSessionActorDied,
        EvSessionActorDied
    >;

    using TEvVolumePipeReset = TRequestEvent<
        TVolumePipeReset,
        EvVolumePipeReset
    >;

    using TEvResetPipeClient = TRequestEvent<
        TResetPipeClient,
        EvResetPipeClient
    >;
};

}   // namespace NCloud::NBlockStore::NStorage
