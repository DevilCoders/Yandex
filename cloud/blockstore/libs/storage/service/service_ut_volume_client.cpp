#include "service_ut.h"

#include <cloud/blockstore/libs/diagnostics/config.h>
#include <cloud/blockstore/libs/storage/api/service.h>
#include <cloud/blockstore/libs/storage/api/ss_proxy.h>
#include <cloud/blockstore/libs/storage/api/volume.h>
#include <cloud/blockstore/libs/storage/api/volume_proxy.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/proto_helpers.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TVolumeClientTest)
{
    Y_UNIT_TEST(ShouldNotifyOwnerAboutTabletConnectionDrop)
    {
        TTestEnv env;
        ui32 nodeIdx = SetupTestEnvWithMultipleMount(
            env,
            TDuration::Seconds(10));

        auto& runtime = env.GetRuntime();
        TServiceClient service(runtime, nodeIdx);
        service.CreateVolume();

        ui64 volumeTabletId = 0;
        bool pipeResetSeen = false;
        TActorId pipeClientActorId;
        TActorId volumeClientActorId;
        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvSSProxy::EvDescribeVolumeResponse: {
                        auto* msg = event->Get<TEvSSProxy::TEvDescribeVolumeResponse>();
                        const auto& volumeDescription =
                            msg->PathDescription.GetBlockStoreVolumeDescription();
                        volumeTabletId = volumeDescription.GetVolumeTabletId();
                        break;
                    }
                    case TEvTabletPipe::EvClientConnected: {
                        pipeClientActorId = event->Sender;
                        volumeClientActorId = event->Recipient;
                        break;
                    }
                    case TEvServicePrivate::EvVolumePipeReset: {
                        pipeResetSeen = true;
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        service.MountVolume();
        UNIT_ASSERT(volumeTabletId);

        auto msg = std::make_unique<TEvTabletPipe::TEvClientDestroyed>(
            (ui64)0,
            TActorId(),
            TActorId());
        service.SendRequest(volumeClientActorId, std::move(msg));

        runtime.DispatchEvents({}, TDuration::Seconds(1));
        UNIT_ASSERT(pipeResetSeen);
    }
}

}   // namespace NCloud::NBlockStore::NStorage

