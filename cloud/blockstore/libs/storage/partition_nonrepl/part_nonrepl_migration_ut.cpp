#include "part_nonrepl_migration_actor.h"
#include "ut_env.h"

#include <cloud/blockstore/libs/common/sglist_test.h>
#include <cloud/blockstore/libs/rdma_test/client_test.h>
#include <cloud/blockstore/libs/diagnostics/block_digest.h>
#include <cloud/blockstore/libs/diagnostics/profile_log.h>
#include <cloud/blockstore/libs/storage/api/disk_agent.h>
#include <cloud/blockstore/libs/storage/api/volume.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/protos/disk.pb.h>
#include <cloud/blockstore/libs/storage/testlib/disk_agent_mock.h>

#include <ydb/core/testlib/basics/runtime.h>
#include <ydb/core/testlib/tablet_helpers.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/datetime/base.h>
#include <util/generic/size_literals.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;
using namespace NKikimr;

namespace {

////////////////////////////////////////////////////////////////////////////////

using TMigrations =
    google::protobuf::RepeatedPtrField<NProto::TDeviceMigration>;

struct TTestEnv
{
    TTestActorRuntime& Runtime;
    TActorId ActorId;
    TActorId VolumeActorId;
    TStorageStatsServiceStatePtr StorageStatsServiceState;
    TDiskAgentStatePtr DiskAgentState;
    NRdma::IClientPtr RdmaClient;

    static void InitDevice(
        ui32 nodeId,
        ui32 blockCount,
        TString name,
        NProto::TDeviceConfig* device)
    {
        const auto k = DefaultBlockSize / DefaultDeviceBlockSize;

        device->SetNodeId(nodeId);
        device->SetBlocksCount(blockCount * k);
        device->SetDeviceUUID(name);
        device->SetBlockSize(DefaultDeviceBlockSize);
        device->SetAgentId(Sprintf("agent-%u", nodeId));
    }

    static TDevices DefaultDevices(ui64 nodeId)
    {
        TDevices devices;
        InitDevice(nodeId, 2048, "vasya", devices.Add());
        InitDevice(nodeId, 3072, "petya", devices.Add());

        return devices;
    }

    static TMigrations DefaultMigrations(ui64 nodeId)
    {
        TMigrations migrations;
        auto* m = migrations.Add();
        m->SetSourceDeviceId("petya");
        auto* d = m->MutableTargetDevice();
        InitDevice(nodeId, 3072, "petya_migration", d);

        return migrations;
    }

    explicit TTestEnv(
            TTestActorRuntime& runtime,
            TDevices devices,
            TMigrations migrations,
            NProto::EVolumeIOMode ioMode,
            bool useRdma)
        : Runtime(runtime)
        , ActorId(0, "YYY")
        , VolumeActorId(0, "VVV")
        , StorageStatsServiceState(MakeIntrusive<TStorageStatsServiceState>())
        , DiskAgentState(std::make_shared<TDiskAgentState>())
    {
        if (useRdma) {
            RdmaClient = std::make_shared<TRdmaClientTest>();
        }

        SetupLogging();

        NProto::TStorageServiceConfig storageConfig;
        storageConfig.SetMaxTimedOutDeviceStateDuration(20'000);
        storageConfig.SetNonReplicatedMinRequestTimeout(1'000);
        storageConfig.SetNonReplicatedMaxRequestTimeout(5'000);

        auto config = std::make_shared<TStorageConfig>(
            std::move(storageConfig),
            std::make_shared<TFeaturesConfig>(NProto::TFeaturesConfig())
        );

        auto nodeId = Runtime.GetNodeId(0);

        TDevices agentDevices = devices;
        for (auto& m: migrations) {
            if (m.GetTargetDevice().GetDeviceUUID()) {
                *agentDevices.Add() = m.GetTargetDevice();

                ToLogicalBlocks(*m.MutableTargetDevice());
            }
        }

        Runtime.SetRegistrationObserverFunc(
            [] (auto& runtime, const auto& parentId, const auto& actorId)
        {
            Y_UNUSED(parentId);
            runtime.EnableScheduleForActor(actorId);
        });

        Runtime.AddLocalService(
            MakeDiskAgentServiceId(nodeId),
            TActorSetupCmd(
                new TDiskAgentMock(agentDevices, DiskAgentState),
                TMailboxType::Simple,
                0
            )
        );

        auto partConfig = std::make_shared<TNonreplicatedPartitionConfig>(
            ToLogicalBlocks(devices),
            ioMode,
            "test",
            DefaultBlockSize,
            VolumeActorId,
            false, // muteIOErrors
            false, // markBlocksUsed
            THashSet<TString>() // freshDeviceIds
        );

        auto part = std::make_unique<TNonreplicatedPartitionMigrationActor>(
            std::move(config),
            CreateProfileLogStub(),
            CreateBlockDigestGeneratorStub(),
            0,
            "", // rwClientId
            std::move(partConfig),
            std::move(migrations),
            RdmaClient
        );

        Runtime.AddLocalService(
            ActorId,
            TActorSetupCmd(part.release(), TMailboxType::Simple, 0)
        );

        auto dummy = std::make_unique<TDummyActor>();

        Runtime.AddLocalService(
            VolumeActorId,
            TActorSetupCmd(dummy.release(), TMailboxType::Simple, 0)
        );

        Runtime.AddLocalService(
            MakeStorageStatsServiceId(),
            TActorSetupCmd(
                new TStorageStatsServiceMock(StorageStatsServiceState),
                TMailboxType::Simple,
                0
            )
        );

        SetupTabletServices(Runtime);
    }

    void SetupLogging()
    {
        Runtime.AppendToLogSettings(
            TBlockStoreComponents::START,
            TBlockStoreComponents::END,
            GetComponentName);

        for (ui32 i = TBlockStoreComponents::START; i < TBlockStoreComponents::END; ++i) {
            Runtime.SetLogPriority(i, NLog::PRI_INFO);
        }
    }

    void KillDiskAgent()
    {
        auto sender = Runtime.AllocateEdgeActor();
        auto nodeId = Runtime.GetNodeId(0);

        auto request = std::make_unique<TEvents::TEvPoisonPill>();

        Runtime.Send(new IEventHandle(
            MakeDiskAgentServiceId(nodeId),
            sender,
            request.release()));

        Runtime.DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));
    }

    TRdmaClientTest& Rdma()
    {
        return static_cast<TRdmaClientTest&>(*RdmaClient);
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TNonreplicatedPartitionMigrationTest)
{
    Y_UNIT_TEST(ShouldMirrorRequestsAfterAllDataIsMigrated)
    {
        TTestBasicRuntime runtime;

        TTestEnv env(
            runtime,
            TTestEnv::DefaultDevices(runtime.GetNodeId(0)),
            TTestEnv::DefaultMigrations(runtime.GetNodeId(0)),
            NProto::VOLUME_IO_OK,
            false);

        TPartitionClient client(runtime, env.ActorId);

        // petya should be migrated => 3 ranges
        WaitForMigrations(runtime, 3);

        const TBlockRange64 blockRange(2048, 2048);
        client.WriteBlocksLocal(blockRange, TString(DefaultBlockSize, 'A'));

        runtime.AdvanceCurrentTime(UpdateCountersInterval);
        runtime.DispatchEvents({}, TDuration::Seconds(1));
        runtime.AdvanceCurrentTime(UpdateCountersInterval);
        runtime.DispatchEvents({}, TDuration::Seconds(1));

        auto& counters = env.StorageStatsServiceState->Counters.RequestCounters;
        UNIT_ASSERT_VALUES_EQUAL(5, counters.WriteBlocks.Count);
        UNIT_ASSERT_VALUES_EQUAL(
            (2 + 3072) * DefaultBlockSize,
            counters.WriteBlocks.RequestBytes
        );
    }

    Y_UNIT_TEST(ShouldDoMigrationViaRdma)
    {
        TTestBasicRuntime runtime;

        TTestEnv env(
            runtime,
            TTestEnv::DefaultDevices(runtime.GetNodeId(0)),
            TTestEnv::DefaultMigrations(runtime.GetNodeId(0)),
            NProto::VOLUME_IO_OK,
            true);

        env.Rdma().InitAllEndpoints();

        // petya should be migrated => 3 ranges
        WaitForMigrations(runtime, 3);
    }

    Y_UNIT_TEST(ShouldDoMigrationEvenInReadOnlyMode)
    {
        TTestBasicRuntime runtime;

        TTestEnv env(
            runtime,
            TTestEnv::DefaultDevices(runtime.GetNodeId(0)),
            TTestEnv::DefaultMigrations(runtime.GetNodeId(0)),
            NProto::VOLUME_IO_ERROR_READ_ONLY,
            false);

        // petya should be migrated => 3 ranges
        WaitForMigrations(runtime, 3);
    }
}

}   // namespace NCloud::NBlockStore::NStorage
