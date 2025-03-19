#include "service_ut.h"

#include <cloud/blockstore/libs/storage/api/volume.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/protos/disk.pb.h>
#include <cloud/blockstore/private/api/protos/checkpoints.pb.h>
#include <cloud/blockstore/private/api/protos/disk.pb.h>
#include <cloud/blockstore/private/api/protos/volume.pb.h>

#include <library/cpp/json/json_writer.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/size_literals.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NKikimr;
using namespace std::string_literals;

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TServiceActionsTest)
{
    Y_UNIT_TEST(ShouldForwardChangeStateRequestsToDiskRegistry)
    {
        auto drState = MakeIntrusive<TDiskRegistryState>();
        TTestEnv env(1, 1, 4, 1, {drState});

        NProto::TStorageServiceConfig config;
        config.SetAllocationUnitNonReplicatedSSD(100);
        ui32 nodeIdx = SetupTestEnv(env, config);

        TServiceClient service(env.GetRuntime(), nodeIdx);
        service.CreateVolume(
            DefaultDiskId,
            100_GB / DefaultBlockSize,
            DefaultBlockSize,
            "",
            "",
            NCloud::NProto::STORAGE_MEDIA_SSD_NONREPLICATED
        );

        auto* disk = drState->Disks.FindPtr(DefaultDiskId);
        UNIT_ASSERT(disk);
        UNIT_ASSERT_VALUES_UNEQUAL(0, disk->Devices.size());
        const auto& device = disk->Devices[0];
        UNIT_ASSERT_VALUES_EQUAL(
            static_cast<ui32>(NProto::DEVICE_STATE_ONLINE),
            static_cast<ui32>(device.GetState())
        );

        {
            NPrivateProto::TDiskRegistryChangeStateRequest request;
            auto* cds = request.MutableChangeDeviceState();
            cds->SetDeviceUUID(device.GetDeviceUUID());
            cds->SetState(static_cast<ui32>(NProto::DEVICE_STATE_ERROR));

            TString buf;
            google::protobuf::util::MessageToJsonString(request, &buf);
            service.SendExecuteActionRequest("DiskRegistryChangeState", buf);

            auto response = service.RecvExecuteActionResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_ARGUMENT, response->GetStatus());

            request.SetMessage("some message");
            buf.clear();
            google::protobuf::util::MessageToJsonString(request, &buf);

            service.ExecuteAction("DiskRegistryChangeState", buf);
        }

        UNIT_ASSERT_VALUES_EQUAL(
            static_cast<ui32>(NProto::DEVICE_STATE_ERROR),
            static_cast<ui32>(device.GetState())
        );

        {
            NPrivateProto::TDiskRegistryChangeStateRequest request;
            request.SetMessage("some message");
            auto* cds = request.MutableChangeDeviceState();
            cds->SetDeviceUUID("nosuchdevice");
            cds->SetState(static_cast<ui32>(NProto::DEVICE_STATE_ERROR));

            TString buf;
            google::protobuf::util::MessageToJsonString(request, &buf);
            service.SendExecuteActionRequest("DiskRegistryChangeState", buf);

            auto response = service.RecvExecuteActionResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_NOT_FOUND, response->GetStatus());
        }

        {
            NPrivateProto::TDiskRegistryChangeStateRequest request;
            request.SetMessage("some message");
            auto* cds = request.MutableChangeAgentState();
            cds->SetAgentId("someagent");
            cds->SetState(static_cast<ui32>(NProto::AGENT_STATE_UNAVAILABLE));

            TString buf;
            google::protobuf::util::MessageToJsonString(request, &buf);
            service.ExecuteAction("DiskRegistryChangeState", buf);
        }

        UNIT_ASSERT_VALUES_EQUAL(1, drState->AgentStates.size());
        UNIT_ASSERT_VALUES_EQUAL(
            "someagent",
            drState->AgentStates[0].first
        );
        UNIT_ASSERT_VALUES_EQUAL(
            static_cast<ui32>(NProto::AGENT_STATE_UNAVAILABLE),
            static_cast<ui32>(drState->AgentStates[0].second)
        );
    }

    Y_UNIT_TEST(ShouldModifyVolumeTagsAndDescribeVolume)
    {
        TTestEnv env;
        NProto::TStorageServiceConfig config;
        ui32 nodeIdx = SetupTestEnv(env, std::move(config));

        TServiceClient service(env.GetRuntime(), nodeIdx);
        service.CreateVolume();

        {
            NPrivateProto::TModifyTagsRequest request;
            request.SetDiskId(DefaultDiskId);
            *request.AddTagsToAdd() = "a";
            *request.AddTagsToAdd() = "b";

            TString buf;
            google::protobuf::util::MessageToJsonString(request, &buf);
            service.ExecuteAction("ModifyTags", buf);
        }

        {
            NPrivateProto::TModifyTagsRequest request;
            request.SetDiskId(DefaultDiskId);
            *request.AddTagsToAdd() = "c";
            *request.AddTagsToRemove() = "b";
            *request.AddTagsToRemove() = "d";

            TString buf;
            google::protobuf::util::MessageToJsonString(request, &buf);
            service.ExecuteAction("ModifyTags", buf);
        }

        {
            NPrivateProto::TModifyTagsRequest request;
            request.SetDiskId(DefaultDiskId);
            *request.AddTagsToAdd() = ",,,";

            TString buf;
            google::protobuf::util::MessageToJsonString(request, &buf);
            service.SendExecuteActionRequest("ModifyTags", buf);
            auto response = service.RecvExecuteActionResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_ARGUMENT, response->GetStatus());
        }

        {
            NPrivateProto::TDescribeVolumeRequest request;
            request.SetDiskId(DefaultDiskId);
            TString buf;
            google::protobuf::util::MessageToJsonString(request, &buf);
            auto response = service.ExecuteAction("DescribeVolume", buf);
            NKikimrSchemeOp::TBlockStoreVolumeDescription pathDescr;
            UNIT_ASSERT(google::protobuf::util::JsonStringToMessage(
                response->Record.GetOutput(),
                &pathDescr
            ).ok());
            const auto& config = pathDescr.GetVolumeConfig();
            UNIT_ASSERT_VALUES_EQUAL("a,c", config.GetTagsStr());
        }

        {
            NPrivateProto::TModifyTagsRequest request;
            request.SetDiskId(DefaultDiskId);
            *request.AddTagsToRemove() = "a";

            TString buf;
            google::protobuf::util::MessageToJsonString(request, &buf);
            service.ExecuteAction("ModifyTags", buf);
        }

        {
            NPrivateProto::TModifyTagsRequest request;
            request.SetDiskId(DefaultDiskId);
            *request.AddTagsToRemove() = "c";

            TString buf;
            google::protobuf::util::MessageToJsonString(request, &buf);
            service.ExecuteAction("ModifyTags", buf);
        }

        {
            NPrivateProto::TDescribeVolumeRequest request;
            request.SetDiskId(DefaultDiskId);
            TString buf;
            google::protobuf::util::MessageToJsonString(request, &buf);
            auto response = service.ExecuteAction("DescribeVolume", buf);
            NKikimrSchemeOp::TBlockStoreVolumeDescription pathDescr;
            UNIT_ASSERT(google::protobuf::util::JsonStringToMessage(
                response->Record.GetOutput(),
                &pathDescr
            ).ok());
            const auto& config = pathDescr.GetVolumeConfig();
            UNIT_ASSERT_VALUES_EQUAL("", config.GetTagsStr());
        }

        service.DestroyVolume();
    }

    Y_UNIT_TEST(ShouldAllowToChangeBaseDiskId)
    {
        TTestEnv env;
        NProto::TStorageServiceConfig config;
        ui32 nodeIdx = SetupTestEnv(env, std::move(config));

        TServiceClient service(env.GetRuntime(), nodeIdx);
        service.CreateVolume(DefaultDiskId);
        service.CreateVolume("new-base-disk");

        {
            NPrivateProto::TRebaseVolumeRequest rebaseReq;
            rebaseReq.SetDiskId(DefaultDiskId);
            rebaseReq.SetTargetBaseDiskId("new-base-disk");
            rebaseReq.SetConfigVersion(1);

            TString buf;
            google::protobuf::util::MessageToJsonString(rebaseReq, &buf);
            service.ExecuteAction("rebasevolume", buf);
        }

        {
            NPrivateProto::TDescribeVolumeRequest request;
            request.SetDiskId(DefaultDiskId);
            TString buf;
            google::protobuf::util::MessageToJsonString(request, &buf);
            auto response = service.ExecuteAction("DescribeVolume", buf);
            NKikimrSchemeOp::TBlockStoreVolumeDescription pathDescr;
            UNIT_ASSERT(google::protobuf::util::JsonStringToMessage(
                response->Record.GetOutput(),
                &pathDescr
            ).ok());
            const auto& config = pathDescr.GetVolumeConfig();
            UNIT_ASSERT_VALUES_EQUAL("new-base-disk", config.GetBaseDiskId());
        }
    }

    Y_UNIT_TEST(ShouldForwardDeleteCheckpointDataToVolume)
    {
        TTestEnv env;
        NProto::TStorageServiceConfig config;
        ui32 nodeIdx = SetupTestEnv(env, std::move(config));

        TServiceClient service(env.GetRuntime(), nodeIdx);
        service.CreateVolume(DefaultDiskId);
        auto createCheckpointResponse = service.CreateCheckpoint(DefaultDiskId, "c1");
        UNIT_ASSERT_VALUES_EQUAL(S_OK, createCheckpointResponse->GetStatus());

        {
            NPrivateProto::TDeleteCheckpointDataRequest deleteCheckpointDataRequest;
            deleteCheckpointDataRequest.SetDiskId(DefaultDiskId);
            deleteCheckpointDataRequest.SetCheckpointId("c1");

            TString buf;
            google::protobuf::util::MessageToJsonString(deleteCheckpointDataRequest, &buf);
            auto executeResponse = service.ExecuteAction("deletecheckpointdata", buf);
            UNIT_ASSERT_VALUES_EQUAL(S_OK, executeResponse->GetStatus());

            NPrivateProto::TDeleteCheckpointDataResponse deleteCheckpointDataResponse;
            UNIT_ASSERT(google::protobuf::util::JsonStringToMessage(
                executeResponse->Record.GetOutput(),
                &deleteCheckpointDataResponse
            ).ok());
            UNIT_ASSERT_VALUES_EQUAL(S_OK, deleteCheckpointDataResponse.GetError().GetCode());
        }

        auto deleteCheckpointResponse = service.DeleteCheckpoint(DefaultDiskId, "c1");
        UNIT_ASSERT_VALUES_EQUAL(S_OK, deleteCheckpointResponse->GetStatus());
    }

    Y_UNIT_TEST(ShouldForwardAllowDiskAllocationRequestsToDiskRegistry)
    {
        auto drState = MakeIntrusive<TDiskRegistryState>();
        TTestEnv env(1, 1, 4, 1, {drState});

        NProto::TStorageServiceConfig config;
        ui32 nodeIdx = SetupTestEnv(env, std::move(config));

        TServiceClient service(env.GetRuntime(), nodeIdx);

        UNIT_ASSERT(!drState->AllowDiskAllocation);

        NProto::TAllowDiskAllocationRequest request;
        request.SetAllow(true);

        TString buf;
        google::protobuf::util::MessageToJsonString(request, &buf);

        service.ExecuteAction("allowdiskallocation", buf);

        UNIT_ASSERT(drState->AllowDiskAllocation);
    }

    Y_UNIT_TEST(ShouldForwardBackupRequestsToDiskRegistry)
    {
        auto drState = MakeIntrusive<TDiskRegistryState>();
        TTestEnv env(1, 1, 4, 1, {drState});

        NProto::TStorageServiceConfig config;
        config.SetAllocationUnitNonReplicatedSSD(100);
        ui32 nodeIdx = SetupTestEnv(env, config);

        TServiceClient service(env.GetRuntime(), nodeIdx);
        service.CreateVolume(
            DefaultDiskId,
            100_GB / DefaultBlockSize,
            DefaultBlockSize,
            "",
            "",
            NCloud::NProto::STORAGE_MEDIA_SSD_NONREPLICATED
        );

        NProto::TBackupDiskRegistryStateResponse request;

        TString buf;
        google::protobuf::util::MessageToJsonString(request, &buf);

        auto response = service.ExecuteAction("backupdiskregistrystate", buf);

        NProto::TBackupDiskRegistryStateResponse proto;
        UNIT_ASSERT(google::protobuf::util::JsonStringToMessage(
            response->Record.GetOutput(),
            &proto
        ).ok());
        const auto& backup = proto.GetBackup();

        UNIT_ASSERT_VALUES_EQUAL(1, backup.DisksSize());
        UNIT_ASSERT_VALUES_EQUAL(DefaultDiskId, backup.GetDisks(0).GetDiskId());
    }

    Y_UNIT_TEST(ShouldForwardUpdatePlacementGroupSettingsRequestsToDiskRegistry)
    {
        auto drState = MakeIntrusive<TDiskRegistryState>();
        TTestEnv env(1, 1, 4, 1, {drState});

        NProto::TStorageServiceConfig config;
        ui32 nodeIdx = SetupTestEnv(env, config);

        TServiceClient service(env.GetRuntime(), nodeIdx);
        service.CreatePlacementGroup("group-1");

        NProto::TUpdatePlacementGroupSettingsRequest request;
        request.SetGroupId("group-1");
        request.SetConfigVersion(1);
        request.MutableSettings()->SetMaxDisksInGroup(100);

        TString buf;
        google::protobuf::util::MessageToJsonString(request, &buf);

        auto executeResponse = service.ExecuteAction("updateplacementgroupsettings", buf);
        UNIT_ASSERT_VALUES_EQUAL(S_OK, executeResponse->GetStatus());

        NProto::TBackupDiskRegistryStateResponse response;
        UNIT_ASSERT(google::protobuf::util::JsonStringToMessage(
            executeResponse->Record.GetOutput(),
            &response
        ).ok());

        UNIT_ASSERT_VALUES_EQUAL(S_OK, response.GetError().GetCode());
        const auto* group = drState->PlacementGroups.FindPtr("group-1");
        UNIT_ASSERT(group);
        UNIT_ASSERT_VALUES_EQUAL(100, group->Settings.GetMaxDisksInGroup());
    }

    Y_UNIT_TEST(ShouldRebindLocalVolumes)
    {
        TTestEnv env;
        NProto::TStorageServiceConfig config;
        ui32 nodeIdx = SetupTestEnv(env, std::move(config));

        TServiceClient service(env.GetRuntime(), nodeIdx);
        service.CreateVolume("vol0");   // local
        service.CreateVolume("vol1");   // local
        service.CreateVolume("vol2");   // remote
        service.CreateVolume("vol3");   // not mounted

        service.MountVolume("vol0");
        service.MountVolume("vol1");
        service.MountVolume(
            "vol2",
            TString(),  // instanceId
            TString(),  // token
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_REMOTE
        );

        TVector<TString> diskIds;

        env.GetRuntime().SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvService::EvChangeVolumeBindingRequest: {
                        auto* msg = event->Get<TEvService::TEvChangeVolumeBindingRequest>();
                        UNIT_ASSERT_VALUES_EQUAL(
                            static_cast<ui32>(TEvService::TChangeVolumeBindingRequest::EChangeBindingOp::RELEASE_TO_HIVE),
                            static_cast<ui32>(msg->Action)
                        );

                        diskIds.push_back(msg->DiskId);
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            }
        );

        NPrivateProto::TRebindVolumesRequest request;
        request.SetBinding(2);  // REMOTE

        TString buf;
        google::protobuf::util::MessageToJsonString(request, &buf);

        service.ExecuteAction("rebindvolumes", buf);

        Sort(diskIds.begin(), diskIds.end());
        // 1. request sent from action handler to service
        // 2. request sent from service to volume session actor
        // => 2 requests for each volume
        UNIT_ASSERT_VALUES_EQUAL(4, diskIds.size());
        UNIT_ASSERT_VALUES_EQUAL("vol0", diskIds[0]);
        UNIT_ASSERT_VALUES_EQUAL("vol0", diskIds[1]);
        UNIT_ASSERT_VALUES_EQUAL("vol1", diskIds[2]);
        UNIT_ASSERT_VALUES_EQUAL("vol1", diskIds[3]);
    }

    Y_UNIT_TEST(ShouldDrainNode)
    {
        TTestEnv env;
        NProto::TStorageServiceConfig config;
        ui32 nodeIdx = SetupTestEnv(env, std::move(config));

        TServiceClient service(env.GetRuntime(), nodeIdx);

        ui64 observedNodeIdx = 0;
        bool observedKeepDown = false;

        env.GetRuntime().SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvHive::EvDrainNode: {
                        auto* msg = event->Get<TEvHive::TEvDrainNode>();
                        observedNodeIdx = msg->Record.GetNodeID();
                        observedKeepDown = msg->Record.GetKeepDown();
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            }
        );

        NPrivateProto::TDrainNodeRequest request;

        TString buf;
        google::protobuf::util::MessageToJsonString(request, &buf);

        service.ExecuteAction("drainnode", buf);

        UNIT_ASSERT_VALUES_EQUAL(
            service.GetSender().NodeId(),
            observedNodeIdx
        );

        UNIT_ASSERT(!observedKeepDown);

        request.SetKeepDown(true);

        buf.clear();
        google::protobuf::util::MessageToJsonString(request, &buf);

        service.ExecuteAction("drainnode", buf);

        UNIT_ASSERT_VALUES_EQUAL(
            service.GetSender().NodeId(),
            observedNodeIdx
        );

        UNIT_ASSERT(observedKeepDown);
    }

    Y_UNIT_TEST(ShouldForwardUpdateUsedBlocksToVolume)
    {
        TTestEnv env;
        NProto::TStorageServiceConfig config;
        ui32 nodeIdx = SetupTestEnv(env, std::move(config));

        TServiceClient service(env.GetRuntime(), nodeIdx);
        service.CreateVolume(DefaultDiskId);

        env.GetRuntime().SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvVolume::EvUpdateUsedBlocksRequest: {
                        auto* msg =
                            event->Get<TEvVolume::TEvUpdateUsedBlocksRequest>();
                        UNIT_ASSERT_VALUES_EQUAL(1, msg->Record.StartIndicesSize());
                        UNIT_ASSERT_VALUES_EQUAL(1, msg->Record.BlockCountsSize());
                        UNIT_ASSERT_VALUES_EQUAL(11, msg->Record.GetStartIndices(0));
                        UNIT_ASSERT_VALUES_EQUAL(22, msg->Record.GetBlockCounts(0));
                        UNIT_ASSERT_VALUES_EQUAL(true, msg->Record.GetUsed());
                    }
                }

                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            }
        );

        {
            NPrivateProto::TModifyTagsRequest request;
            request.SetDiskId(DefaultDiskId);
            *request.AddTagsToAdd() = "track-used";

            TString buf;
            google::protobuf::util::MessageToJsonString(request, &buf);
            service.ExecuteAction("ModifyTags", buf);
        }

        {
            NProto::TUpdateUsedBlocksRequest request;
            request.SetDiskId(DefaultDiskId);
            request.AddStartIndices(11);
            request.AddBlockCounts(22);
            request.SetUsed(true);

            TString buf;
            google::protobuf::util::MessageToJsonString(request, &buf);
            auto executeResponse = service.ExecuteAction("updateusedblocks", buf);
            UNIT_ASSERT_VALUES_EQUAL(S_OK, executeResponse->GetStatus());

            NProto::TUpdateUsedBlocksResponse response;
            UNIT_ASSERT(google::protobuf::util::JsonStringToMessage(
                executeResponse->Record.GetOutput(),
                &response
            ).ok());

            UNIT_ASSERT_VALUES_EQUAL(S_OK, response.GetError().GetCode());
        }
    }

    Y_UNIT_TEST(ShouldRebuildMetadataForPartitionVersion1)
    {
        TTestEnv env;
        NProto::TStorageServiceConfig config;
        ui32 nodeIdx = SetupTestEnv(env, std::move(config));

        TServiceClient service(env.GetRuntime(), nodeIdx);
        service.CreateVolume("vol0");

        auto sessionId = service.MountVolume("vol0")->Record.GetSessionId();

        service.WriteBlocks(
            "vol0",
            TBlockRange64(0, 1023),
            sessionId,
            char(1));

        {
            NPrivateProto::TRebuildMetadataRequest request;
            request.SetDiskId("vol0");
            request.SetMetadataType(NPrivateProto::USED_BLOCKS);
            request.SetBatchSize(100);

            TString buf;
            google::protobuf::util::MessageToJsonString(request, &buf);

            auto response = service.ExecuteAction("rebuildmetadata", buf);
            NPrivateProto::TRebuildMetadataResponse metadataResponse;
            UNIT_ASSERT(google::protobuf::util::JsonStringToMessage(
                response->Record.GetOutput(),
                &metadataResponse
            ).ok());
        }

        {
            NPrivateProto::TGetRebuildMetadataStatusRequest request;
            request.SetDiskId("vol0");

            TString buf;
            google::protobuf::util::MessageToJsonString(request, &buf);

            auto response = service.ExecuteAction("getrebuildmetadatastatus", buf);
            NPrivateProto::TGetRebuildMetadataStatusResponse metadataResponse;

            UNIT_ASSERT(google::protobuf::util::JsonStringToMessage(
                response->Record.GetOutput(),
                &metadataResponse
            ).ok());

            UNIT_ASSERT_VALUES_EQUAL(1024, metadataResponse.GetProgress().GetTotal());
        }
    }

    Y_UNIT_TEST(ShouldRebuildMetadataBlockCountForPartitionVersion1)
    {
        TTestEnv env;
        NProto::TStorageServiceConfig config;
        ui32 nodeIdx = SetupTestEnv(env, std::move(config));

        TServiceClient service(env.GetRuntime(), nodeIdx);
        service.CreateVolume("vol0");

        auto sessionId = service.MountVolume("vol0")->Record.GetSessionId();

        service.WriteBlocks(
            "vol0",
            TBlockRange64(0, 1023),
            sessionId,
            char(1));

        {
            NPrivateProto::TRebuildMetadataRequest request;
            request.SetDiskId("vol0");
            request.SetMetadataType(NPrivateProto::BLOCK_COUNT);
            request.SetBatchSize(100);

            TString buf;
            google::protobuf::util::MessageToJsonString(request, &buf);

            auto response = service.ExecuteAction("rebuildmetadata", buf);
            NPrivateProto::TRebuildMetadataResponse metadataResponse;
            UNIT_ASSERT(google::protobuf::util::JsonStringToMessage(
                response->Record.GetOutput(),
                &metadataResponse
            ).ok());
        }

        {
            NPrivateProto::TGetRebuildMetadataStatusRequest request;
            request.SetDiskId("vol0");

            TString buf;
            google::protobuf::util::MessageToJsonString(request, &buf);

            auto response = service.ExecuteAction("getrebuildmetadatastatus", buf);
            NPrivateProto::TGetRebuildMetadataStatusResponse metadataResponse;
            UNIT_ASSERT(google::protobuf::util::JsonStringToMessage(
                response->Record.GetOutput(),
                &metadataResponse
            ).ok());

            UNIT_ASSERT_VALUES_EQUAL(1, metadataResponse.GetProgress().GetTotal());
        }
    }

    Y_UNIT_TEST(ShouldCreateDiskFromDevices)
    {
        const TString agentId = "agent.yandex.cloud.net";

        auto createDevice = [&] (TString name, TString uuid) {
            NProto::TDeviceConfig device;

            device.SetAgentId(agentId);
            device.SetDeviceName(std::move(name));
            device.SetDeviceUUID(std::move(uuid));
            device.SetBlocksCount(93_GB);
            device.SetBlockSize(4096);

            return device;
        };

        auto diskRegistryState = MakeIntrusive<TDiskRegistryState>();

        diskRegistryState->Devices = {
            createDevice("/dev/nvme3n1", "uuid-1"),
            createDevice("/dev/nvme3n2", "uuid-2"),
            createDevice("/dev/nvme3n3", "uuid-3"),
            createDevice("/dev/nvme3n4", "uuid-4")
        };

        TTestEnv env {1, 1, 4, 1, {diskRegistryState}};

        TServiceClient service {env.GetRuntime(), SetupTestEnv(env)};

        service.ExecuteAction("CreateDiskFromDevices", WriteJson(
            NJson::TJsonMap {
                {"DiskId"s, "vol0"s},
                {"BlockSize"s, 8192},
                {"Devices"s, NJson::TJsonArray {
                    NJson::TJsonMap {
                        {"AgentId"s, agentId},
                        {"Path"s, "/dev/nvme3n1"s}
                    },
                    NJson::TJsonMap {
                        {"AgentId"s, agentId},
                        {"Path"s, "/dev/nvme3n3"s}
                    }
                }}
            }));

        UNIT_ASSERT_VALUES_EQUAL(1, diskRegistryState->Disks.size());
        UNIT_ASSERT(diskRegistryState->Disks.contains("vol0"));
        UNIT_ASSERT_VALUES_EQUAL(
            2,
            diskRegistryState->Disks["vol0"].Devices.size());

        UNIT_ASSERT_VALUES_EQUAL(
            8192,
            diskRegistryState->Disks["vol0"].BlockSize);

        UNIT_ASSERT_VALUES_EQUAL(
            "uuid-1",
            diskRegistryState->Disks["vol0"].Devices[0].GetDeviceUUID());

        UNIT_ASSERT_VALUES_EQUAL(
            "uuid-3",
            diskRegistryState->Disks["vol0"].Devices[1].GetDeviceUUID());
    }
}

}   // namespace NCloud::NBlockStore::NStorage
