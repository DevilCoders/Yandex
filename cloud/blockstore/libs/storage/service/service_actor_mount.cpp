#include "service_actor.h"

#include <cloud/blockstore/libs/diagnostics/critical_events.h>
#include <cloud/blockstore/libs/kikimr/trace.h>
#include <cloud/blockstore/libs/storage/api/undelivered.h>
#include <cloud/blockstore/libs/storage/core/proto_helpers.h>
#include <cloud/blockstore/libs/storage/core/request_info.h>

#include <ydb/core/tablet/tablet_setup.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;

////////////////////////////////////////////////////////////////////////////////

void TServiceActor::HandleMountVolume(
    const TEvService::TEvMountVolumeRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();
    const auto& diskId = GetDiskId(*msg);

    BLOCKSTORE_TRACE_RECEIVED(ctx, &ev->TraceId, this, msg);

    auto volume = State.GetOrAddVolume(diskId);

    if (Config->GetRemoteMountOnly()) {
        msg->Record.SetVolumeMountMode(NProto::VOLUME_MOUNT_REMOTE);
    }

    if (!volume->VolumeSessionActor) {
        volume->VolumeSessionActor = NCloud::RegisterLocal(
            ctx,
            CreateVolumeSessionActor(
                volume,
                Config,
                DiagnosticsConfig,
                ProfileLog,
                BlockDigestGenerator,
                TraceSerializer,
                RdmaClient,
                Counters,
                SharedCounters
            )
        );
    }
    ForwardRequestWithNondeliveryTracking(ctx, volume->VolumeSessionActor, *ev);
}

////////////////////////////////////////////////////////////////////////////////

void TServiceActor::HandleSessionActorDied(
    const TEvServicePrivate::TEvSessionActorDied::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ctx);

    auto* msg = ev->Get();
    const auto& diskId = msg->DiskId;

    auto volume = State.GetVolume(diskId);

    if (volume) {
        State.RemoveVolume(volume);
    }
}

////////////////////////////////////////////////////////////////////////////////

}   // namespace NCloud::NBlockStore::NStorage
