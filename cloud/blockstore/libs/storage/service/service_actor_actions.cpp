#include "service_actor.h"

#include <cloud/blockstore/libs/storage/core/probes.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER)

////////////////////////////////////////////////////////////////////////////////

void TServiceActor::HandleExecuteAction(
    const TEvService::TEvExecuteActionRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();
    auto& request = msg->Record;

    TString action = request.GetAction();
    action.to_lower();

    auto& input = *request.MutableInput();

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    LWTRACK(
        RequestReceived_Service,
        msg->CallContext->LWOrbit,
        "ExecuteAction_" + action,
        msg->CallContext->RequestId);

    using TFunc = NActors::IActorPtr(TServiceActor::*)(TRequestInfoPtr, TString);
    const THashMap<TString, TFunc> actions = {
        {"allowdiskallocation",          &TServiceActor::CreateAllowDiskAllocationActionActor      },
        {"backupdiskregistrystate",      &TServiceActor::CreateBackupDiskRegistryStateActor        },
        {"checkblob",                    &TServiceActor::CreateCheckBlobActionActor                },
        {"compactrange",                 &TServiceActor::CreateCompactRangeActionActor             },
        {"configurevolumebalancer",      &TServiceActor::CreateConfigureVolumeBalancerActionActor  },
        {"creatediskfromdevices",        &TServiceActor::CreateCreateDiskFromDevices               },
        {"deletecheckpointdata",         &TServiceActor::CreateDeleteCheckpointDataActionActor     },
        {"describeblocks",               &TServiceActor::CreateDescribeBlocksActionActor           },
        {"describevolume",               &TServiceActor::CreateDescribeVolumeActionActor           },
        {"diskregistrychangestate",      &TServiceActor::CreateDiskRegistryChangeStateActor        },
        {"drainnode",                    &TServiceActor::CreateDrainNodeActionActor                },
        {"getcompactionstatus",          &TServiceActor::CreateGetCompactionStatusActionActor      },
        {"getpartitioninfo",             &TServiceActor::CreateGetPartitionInfoActionActor         },
        {"getrebuildmetadatastatus",     &TServiceActor::CreateRebuildMetadataStatusActionActor    },
        {"killtablet",                   &TServiceActor::CreateKillTabletActionActor               },
        {"modifytags",                   &TServiceActor::CreateModifyTagsActionActor               },
        {"reallocatedisk",               &TServiceActor::CreateReallocateDiskActionActor           },
        {"reassigndiskregistry",         &TServiceActor::CreateReassignDiskRegistryActionActor     },
        {"rebasevolume",                 &TServiceActor::CreateRebaseVolumeActionActor             },
        {"rebindvolumes",                &TServiceActor::CreateRebindVolumesActionActor            },
        {"rebuildmetadata",              &TServiceActor::CreateRebuildMetadataActionActor          },
        {"replacedevice",                &TServiceActor::CreateReplaceDeviceActionActor            },
        {"resettablet",                  &TServiceActor::CreateResetTabletActionActor              },
        {"restorediskregistrystate",     &TServiceActor::CreateRestoreDiskRegistryStateActor       },
        {"resumedevice",                 &TServiceActor::CreateResumeDeviceActionActor             },
        {"setuserid",                    &TServiceActor::CreateSetUserIdActionActor                },
        {"suspenddevice",                &TServiceActor::CreateSuspendDeviceActionActor            },
        {"updatediskblocksize",          &TServiceActor::CreateUpdateDiskBlockSizeActionActor      },
        {"updatediskreplicacount",       &TServiceActor::CreateUpdateDiskReplicaCountActionActor   },
        {"updateplacementgroupsettings", &TServiceActor::CreateUpdatePlacementGroupSettingsActor   },
        {"updateusedblocks",             &TServiceActor::CreateUpdateUsedBlocksActionActor         }
    };

    auto it = actions.find(action);
    if (it != actions.end()) {
        auto action = it->second;
        auto actor = std::invoke(
            action,
            this,
            std::move(requestInfo),
            std::move(input)
        );
        NCloud::Register(ctx, std::move(actor));
        return;
    }

    auto response = std::make_unique<TEvService::TEvExecuteActionResponse>(
        MakeError(E_ARGUMENT, "No suitable action found"));

    LWTRACK(
        ResponseSent_Service,
        msg->CallContext->LWOrbit,
        "ExecuteAction_" + action,
        msg->CallContext->RequestId);

    NCloud::Reply(ctx, *requestInfo, std::move(response));
}

}   // namespace NCloud::NBlockStore::NStorage
