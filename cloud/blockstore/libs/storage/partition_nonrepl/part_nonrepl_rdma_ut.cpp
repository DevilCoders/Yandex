#include "part_nonrepl_rdma.h"
#include "part_nonrepl_rdma_actor.h"
#include "ut_env.h"

#include <cloud/blockstore/libs/common/sglist_test.h>
#include <cloud/blockstore/libs/rdma_test/client_test.h>
#include <cloud/blockstore/libs/storage/api/disk_agent.h>
#include <cloud/blockstore/libs/storage/api/volume.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/protos/disk.pb.h>
#include <cloud/blockstore/libs/storage/testlib/disk_agent_mock.h>
#include <cloud/blockstore/libs/storage/api/stats_service.h>

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

struct TTestEnv
{
    TTestActorRuntime& Runtime;
    TActorId ActorId;
    TActorId VolumeActorId;
    TStorageStatsServiceStatePtr StorageStatsServiceState;
    TDiskAgentStatePtr DiskAgentState;
    NRdma::IClientPtr RdmaClient;

    static void AddDevice(
        ui32 nodeId,
        ui32 blockCount,
        TString name,
        TDevices& devices)
    {
        const auto k = DefaultBlockSize / DefaultDeviceBlockSize;

        auto& device = *devices.Add();
        device.SetNodeId(nodeId);
        device.SetBlocksCount(blockCount * k);
        device.SetDeviceUUID(name);
        device.SetBlockSize(DefaultDeviceBlockSize);
        device.SetAgentId(Sprintf("agent-%u", nodeId));
    }

    static TDevices DefaultDevices(ui64 nodeId)
    {
        TDevices devices;
        AddDevice(nodeId, 2048, "vasya", devices);
        AddDevice(nodeId, 3072, "petya", devices);
        AddDevice(0, 1024, "", devices);

        return devices;
    }

    explicit TTestEnv(TTestActorRuntime& runtime)
        : TTestEnv(runtime, NProto::VOLUME_IO_OK)
    {}

    explicit TTestEnv(
            TTestActorRuntime& runtime,
            NProto::EVolumeIOMode ioMode)
        : TTestEnv(
            runtime,
            ioMode,
            DefaultDevices(runtime.GetNodeId(0))
        )
    {}

    explicit TTestEnv(
            TTestActorRuntime& runtime,
            NProto::EVolumeIOMode ioMode,
            TDevices devices)
        : Runtime(runtime)
        , ActorId(0, "YYY")
        , VolumeActorId(0, "VVV")
        , StorageStatsServiceState(MakeIntrusive<TStorageStatsServiceState>())
        , DiskAgentState(std::make_shared<TDiskAgentState>())
        , RdmaClient(std::make_shared<TRdmaClientTest>())
    {
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

        Runtime.AddLocalService(
            MakeDiskAgentServiceId(nodeId),
            TActorSetupCmd(
                new TDiskAgentMock(devices, DiskAgentState),
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

        auto part = std::make_unique<TNonreplicatedPartitionRdmaActor>(
            std::move(config),
            std::move(partConfig),
            RdmaClient,
            VolumeActorId
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

        // for (ui32 i = TBlockStoreComponents::START; i < TBlockStoreComponents::END; ++i) {
        //    Runtime.SetLogPriority(i, NLog::PRI_DEBUG);
        // }
        // Runtime.SetLogPriority(NLog::InvalidComponent, NLog::PRI_DEBUG);
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

Y_UNIT_TEST_SUITE(TNonreplicatedPartitionRdmaTest)
{
    Y_UNIT_TEST(ShouldLocalReadWrite)
    {
        TTestBasicRuntime runtime;

        TTestEnv env(runtime);
        TPartitionClient client(runtime, env.ActorId);

        const TBlockRange64 blockRange1(1024, 4095);
        client.SendWriteBlocksLocalRequest(
            blockRange1,
            TString(DefaultBlockSize, 'A'));
        {
            auto response = client.RecvWriteBlocksLocalResponse();
            UNIT_ASSERT_VALUES_EQUAL_C(
                E_REJECTED,
                response->GetStatus(),
                response->GetErrorReason());
        }

        env.Rdma().InitAllEndpoints();

        client.WriteBlocksLocal(blockRange1, TString(DefaultBlockSize, 'A'));

        {
            TVector<TString> blocks;

            client.ReadBlocksLocal(
                blockRange1,
                TGuardedSgList(ResizeBlocks(
                    blocks,
                    blockRange1.Size(),
                    TString(DefaultBlockSize, '\0')
                )));

            for (ui32 i = 0; i < blocks.size(); ++i) {
                UNIT_ASSERT_VALUES_EQUAL_C(
                    TString(4096, 'A'),
                    blocks[i],
                    TStringBuilder() << "block " << i);
            }
        }

        const TBlockRange64 blockRange2(5000, 5199);
        client.WriteBlocksLocal(blockRange2, TString(DefaultBlockSize, 'B'));

        const TBlockRange64 blockRange3(5000, 5150);

        {
            TVector<TString> blocks;

            client.ReadBlocksLocal(
                blockRange3,
                TGuardedSgList(ResizeBlocks(
                    blocks,
                    blockRange3.Size(),
                    TString(DefaultBlockSize, '\0')
                )));

            for (ui32 i = 0; i < 120; ++i) {
                UNIT_ASSERT_VALUES_EQUAL_C(
                    TString(4096, 'B'),
                    blocks[i],
                    TStringBuilder() << "block " << i);
            }

            for (ui32 i = 120; i < blockRange3.Size(); ++i) {
                UNIT_ASSERT_VALUES_EQUAL_C(
                    TString(4096, 0),
                    blocks[i],
                    TStringBuilder() << "block " << i);
            }
        }

        client.ZeroBlocks(blockRange3);

        {
            TVector<TString> blocks;

            client.ReadBlocksLocal(
                blockRange3,
                TGuardedSgList(ResizeBlocks(
                    blocks,
                    blockRange3.Size(),
                    TString(DefaultBlockSize, '\0')
                )));

            for (ui32 i = 0; i < blockRange3.Size(); ++i) {
                UNIT_ASSERT_VALUES_EQUAL_C(
                    TString(4096, 0),
                    blocks[i],
                    TStringBuilder() << "block " << i);
            }
        }

        client.SendRequest(
            env.ActorId,
            std::make_unique<TEvNonreplPartitionPrivate::TEvUpdateCounters>()
        );

        runtime.DispatchEvents({}, TDuration::Seconds(1));

        auto& counters = env.StorageStatsServiceState->Counters.RequestCounters;
        UNIT_ASSERT_VALUES_EQUAL(3, counters.ReadBlocks.Count);
        UNIT_ASSERT_VALUES_EQUAL(
            DefaultBlockSize * (
                blockRange1.Size() + 2 * blockRange3.Size()
            ),
            counters.ReadBlocks.RequestBytes
        );
        UNIT_ASSERT_VALUES_EQUAL(2, counters.WriteBlocks.Count);
        UNIT_ASSERT_VALUES_EQUAL(
            DefaultBlockSize * (
                blockRange1.Size() + blockRange2.Size()
            ),
            counters.WriteBlocks.RequestBytes
        );

        UNIT_ASSERT_VALUES_EQUAL(
            0,
            env.StorageStatsServiceState->Counters.Simple.IORequestsInFlight.Value
        );
    }

    Y_UNIT_TEST(ShouldRemoteReadWrite)
    {
        TTestBasicRuntime runtime;

        TTestEnv env(runtime);
        env.Rdma().InitAllEndpoints();

        TPartitionClient client(runtime, env.ActorId);

        const TBlockRange64 blockRange1(1024, 4095);
        client.WriteBlocks(blockRange1, 'A');

        {
            auto response = client.ReadBlocks(blockRange1);
            const auto& blocks = response->Record.GetBlocks().GetBuffers();

            for (int i = 0; i < blocks.size(); ++i) {
                UNIT_ASSERT_VALUES_EQUAL_C(
                    TString(4096, 'A'),
                    blocks[i],
                    TStringBuilder() << "block " << i);
            }
        }

        const TBlockRange64 blockRange2(5000, 5199);
        client.WriteBlocks(blockRange2, 'B');

        const TBlockRange64 blockRange3(5000, 5150);

        {
            auto response = client.ReadBlocks(blockRange3);
            const auto& blocks = response->Record.GetBlocks().GetBuffers();

            for (ui32 i = 0; i < 120; ++i) {
                UNIT_ASSERT_VALUES_EQUAL_C(
                    TString(4096, 'B'),
                    blocks[i],
                    TStringBuilder() << "block " << i);
            }

            for (ui32 i = 120; i < blockRange3.Size(); ++i) {
                UNIT_ASSERT_VALUES_EQUAL_C(
                    TString(4096, 0),
                    blocks[i],
                    TStringBuilder() << "block " << i);
            }
        }

        client.SendRequest(
            env.ActorId,
            std::make_unique<TEvNonreplPartitionPrivate::TEvUpdateCounters>()
        );

        runtime.DispatchEvents({}, TDuration::Seconds(1));

        auto& counters = env.StorageStatsServiceState->Counters.RequestCounters;
        UNIT_ASSERT_VALUES_EQUAL(2, counters.ReadBlocks.Count);
        UNIT_ASSERT_VALUES_EQUAL(
            DefaultBlockSize * (
                blockRange1.Size() + blockRange3.Size()
            ),
            counters.ReadBlocks.RequestBytes
        );
        UNIT_ASSERT_VALUES_EQUAL(2, counters.WriteBlocks.Count);
        UNIT_ASSERT_VALUES_EQUAL(
            DefaultBlockSize * (
                blockRange1.Size() + blockRange2.Size()
            ),
            counters.WriteBlocks.RequestBytes
        );

        UNIT_ASSERT_VALUES_EQUAL(
            0,
            env.StorageStatsServiceState->Counters.Simple.IORequestsInFlight.Value
        );
    }

#define WRITE_BLOCKS_E(error) {                                                \
            TString data(DefaultBlockSize, 'A');                               \
            client.SendWriteBlocksLocalRequest(blockRange1, data);             \
            {                                                                  \
                auto response = client.RecvWriteBlocksLocalResponse();         \
                UNIT_ASSERT_VALUES_EQUAL_C(                                    \
                    error.GetCode(),                                           \
                    response->GetStatus(),                                     \
                    response->GetErrorReason());                               \
            }                                                                  \
        }                                                                      \
// WRITE_BLOCKS_E

#define ZERO_BLOCKS_E(error) {                                                 \
            client.SendZeroBlocksRequest(blockRange1);                         \
            {                                                                  \
                auto response = client.RecvZeroBlocksResponse();               \
                UNIT_ASSERT_VALUES_EQUAL_C(                                    \
                    error.GetCode(),                                           \
                    response->GetStatus(),                                     \
                    response->GetErrorReason());                               \
            }                                                                  \
        }                                                                      \
// ZERO_BLOCKS_E

#define READ_BLOCKS_E(error, c) {                                              \
            TVector<TString> blocks;                                           \
                                                                               \
            client.SendReadBlocksLocalRequest(                                 \
                blockRange1,                                                   \
                TGuardedSgList(ResizeBlocks(                                   \
                    blocks,                                                    \
                    blockRange1.Size(),                                        \
                    TString(DefaultBlockSize, '\0')                            \
                )));                                                           \
                                                                               \
            auto response = client.RecvReadBlocksLocalResponse();              \
            UNIT_ASSERT_VALUES_EQUAL_C(                                        \
                error.GetCode(),                                               \
                response->GetStatus(),                                         \
                response->GetErrorReason());                                   \
                                                                               \
            if (!HasError(error)) {                                            \
                for (ui32 i = 0; i < blocks.size(); ++i) {                     \
                    UNIT_ASSERT_VALUES_EQUAL_C(                                \
                        TString(4096, c),                                      \
                        blocks[i],                                             \
                        TStringBuilder() << "block " << i);                    \
                }                                                              \
            }                                                                  \
        }                                                                      \
// READ_BLOCKS_E

    Y_UNIT_TEST(ShouldLocalReadWriteWithErrors)
    {
        TTestBasicRuntime runtime;

        TTestEnv env(runtime);
        TPartitionClient client(runtime, env.ActorId);

        env.Rdma().InitAllEndpoints();

        const auto allocationError = MakeError(E_FAIL, "allocation error");
        const auto responseError = MakeError(E_IO, "response error");

        const TBlockRange64 blockRange1(1024, 4095);

        env.Rdma().InjectErrors(allocationError, {});

        WRITE_BLOCKS_E(allocationError);
        ZERO_BLOCKS_E(allocationError);
        READ_BLOCKS_E(allocationError, 0);

        env.Rdma().InjectErrors({}, responseError);

        WRITE_BLOCKS_E(responseError);
        ZERO_BLOCKS_E(responseError);
        READ_BLOCKS_E(responseError, 0);

        env.Rdma().InjectErrors({}, {});

        READ_BLOCKS_E(NProto::TError{}, 0);
        WRITE_BLOCKS_E(NProto::TError{});
        READ_BLOCKS_E(NProto::TError{}, 'A');
        ZERO_BLOCKS_E(NProto::TError{});
        READ_BLOCKS_E(NProto::TError{}, 0);
    }

    Y_UNIT_TEST(ShouldHandleEndpointInitializationFailure)
    {
        TTestBasicRuntime runtime;

        TTestEnv env(runtime);
        TPartitionClient client(runtime, env.ActorId);

        TActorId notifiedActor;
        ui32 notificationCount = 0;

        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvVolume::EvRdmaUnavailable: {
                        notifiedActor = event->Recipient;
                        ++notificationCount;

                        break;
                    }
                }

                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            }
        );

        env.Rdma().InitAllEndpointsWithError();

        const auto error = MakeError(E_REJECTED, "");

        const TBlockRange64 blockRange1(1024, 4095);

        WRITE_BLOCKS_E(error);
        ZERO_BLOCKS_E(error);
        READ_BLOCKS_E(error, 0);

        UNIT_ASSERT_VALUES_EQUAL(env.VolumeActorId, notifiedActor);
        UNIT_ASSERT_VALUES_EQUAL(1, notificationCount);
    }

#undef WRITE_BLOCKS_E
#undef ZERO_BLOCKS_E
#undef READ_BLOCKS_E
}

}   // namespace NCloud::NBlockStore::NStorage
