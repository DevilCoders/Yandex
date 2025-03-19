#include "part_nonrepl.h"
#include "part_nonrepl_actor.h"
#include "ut_env.h"

#include <cloud/blockstore/libs/common/sglist_test.h>
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
        : TTestEnv(runtime, ioMode, false, DefaultDevices(runtime.GetNodeId(0)))
    {}

    explicit TTestEnv(
            TTestActorRuntime& runtime,
            NProto::EVolumeIOMode ioMode,
            TDevices devices)
        : TTestEnv(runtime, ioMode, false, std::move(devices))
    {}

    explicit TTestEnv(
            TTestActorRuntime& runtime,
            NProto::EVolumeIOMode ioMode,
            bool markBlocksUsed)
        : TTestEnv(
            runtime,
            ioMode,
            markBlocksUsed,
            DefaultDevices(runtime.GetNodeId(0))
        )
    {}

    explicit TTestEnv(
            TTestActorRuntime& runtime,
            NProto::EVolumeIOMode ioMode,
            bool markBlocksUsed,
            TDevices devices)
        : Runtime(runtime)
        , ActorId(0, "YYY")
        , VolumeActorId(0, "VVV")
        , StorageStatsServiceState(MakeIntrusive<TStorageStatsServiceState>())
        , DiskAgentState(std::make_shared<TDiskAgentState>())
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
            markBlocksUsed,
            THashSet<TString>() // freshDeviceIds
        );

        auto part = std::make_unique<TNonreplicatedPartitionActor>(
            std::move(config),
            std::move(partConfig),
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
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TNonreplicatedPartitionTest)
{
    Y_UNIT_TEST(ShouldReadWriteZero)
    {
        TTestBasicRuntime runtime;

        TTestEnv env(runtime);

        TPartitionClient client(runtime, env.ActorId);

        {
            auto response = client.ReadBlocks(TBlockRange64(1024, 4095));
            const auto& blocks = response->Record.GetBlocks();

            UNIT_ASSERT_VALUES_EQUAL(3072, blocks.BuffersSize());
            UNIT_ASSERT_VALUES_EQUAL(DefaultBlockSize, blocks.GetBuffers(0).size());
            UNIT_ASSERT_VALUES_EQUAL(
                TString(DefaultBlockSize, 0),
                blocks.GetBuffers(0)
            );

            UNIT_ASSERT_VALUES_EQUAL(DefaultBlockSize, blocks.GetBuffers(3071).size());
            UNIT_ASSERT_VALUES_EQUAL(
                TString(DefaultBlockSize, 0),
                blocks.GetBuffers(3071)
            );
        }

        client.WriteBlocks(TBlockRange64(1024, 4095), 1);
        client.WriteBlocks(TBlockRange64(1024, 4023), 2);

        {
            auto response = client.ReadBlocks(TBlockRange64(1024, 4095));
            const auto& blocks = response->Record.GetBlocks();
            UNIT_ASSERT_VALUES_EQUAL(3072, blocks.BuffersSize());
            UNIT_ASSERT_VALUES_EQUAL(DefaultBlockSize, blocks.GetBuffers(0).size());
            UNIT_ASSERT_VALUES_EQUAL(DefaultBlockSize, blocks.GetBuffers(2999).size());
            UNIT_ASSERT_VALUES_EQUAL(DefaultBlockSize, blocks.GetBuffers(3000).size());
            UNIT_ASSERT_VALUES_EQUAL(DefaultBlockSize, blocks.GetBuffers(3071).size());

            UNIT_ASSERT_VALUES_EQUAL(
                TString(DefaultBlockSize, 2),
                blocks.GetBuffers(0)
            );
            UNIT_ASSERT_VALUES_EQUAL(
                TString(DefaultBlockSize, 2),
                blocks.GetBuffers(2999)
            );
            UNIT_ASSERT_VALUES_EQUAL(
                TString(DefaultBlockSize, 1),
                blocks.GetBuffers(3000)
            );
            UNIT_ASSERT_VALUES_EQUAL(
                TString(DefaultBlockSize, 1),
                blocks.GetBuffers(3071)
            );
        }

        client.ZeroBlocks(TBlockRange64(2024, 3023));

        {
            auto response = client.ReadBlocks(TBlockRange64(1024, 4095));
            const auto& blocks = response->Record.GetBlocks();
            UNIT_ASSERT_VALUES_EQUAL(3072, blocks.BuffersSize());
            UNIT_ASSERT_VALUES_EQUAL(
                TString(DefaultBlockSize, 2),
                blocks.GetBuffers(0)
            );
            UNIT_ASSERT_VALUES_EQUAL(
                TString(DefaultBlockSize, 2),
                blocks.GetBuffers(999)
            );
            UNIT_ASSERT_VALUES_EQUAL(
                TString(DefaultBlockSize, 0),
                blocks.GetBuffers(1000)
            );
            UNIT_ASSERT_VALUES_EQUAL(
                TString(DefaultBlockSize, 0),
                blocks.GetBuffers(1999)
            );
            UNIT_ASSERT_VALUES_EQUAL(
                TString(DefaultBlockSize, 2),
                blocks.GetBuffers(2000)
            );
            UNIT_ASSERT_VALUES_EQUAL(
                TString(DefaultBlockSize, 2),
                blocks.GetBuffers(2999)
            );
            UNIT_ASSERT_VALUES_EQUAL(
                TString(DefaultBlockSize, 1),
                blocks.GetBuffers(3000)
            );
            UNIT_ASSERT_VALUES_EQUAL(
                TString(DefaultBlockSize, 1),
                blocks.GetBuffers(3071)
            );
        }

        client.WriteBlocks(TBlockRange64(5000, 5199), 3);
        client.ZeroBlocks(TBlockRange64(5050, 5150));

        {
            auto response = client.ReadBlocks(TBlockRange64(5000, 5199));
            const auto& blocks = response->Record.GetBlocks();
            UNIT_ASSERT_VALUES_EQUAL(200, blocks.BuffersSize());
            for (ui32 i = 0; i < 50; ++i) {
                UNIT_ASSERT_VALUES_EQUAL(
                    TString(DefaultBlockSize, 3),
                    blocks.GetBuffers(i)
                );
            }

            for (ui32 i = 51; i < 120; ++i) {
                UNIT_ASSERT_VALUES_EQUAL(
                    TString(DefaultBlockSize, 0),
                    blocks.GetBuffers(i)
                );
            }

            for (ui32 i = 120; i < 200; ++i) {
                UNIT_ASSERT_VALUES_EQUAL(
                    TString(),
                    blocks.GetBuffers(i)
                );
            }
        }

        client.SendRequest(
            env.ActorId,
            std::make_unique<TEvNonreplPartitionPrivate::TEvUpdateCounters>()
        );

        runtime.DispatchEvents({}, TDuration::Seconds(1));

        auto& counters = env.StorageStatsServiceState->Counters.RequestCounters;
        UNIT_ASSERT_VALUES_EQUAL(4, counters.ReadBlocks.Count);
        UNIT_ASSERT_VALUES_EQUAL(
            DefaultBlockSize * 9336,
            counters.ReadBlocks.RequestBytes
        );
        UNIT_ASSERT_VALUES_EQUAL(3, counters.WriteBlocks.Count);
        UNIT_ASSERT_VALUES_EQUAL(
            DefaultBlockSize * 6192,
            counters.WriteBlocks.RequestBytes
        );
        UNIT_ASSERT_VALUES_EQUAL(2, counters.ZeroBlocks.Count);
        UNIT_ASSERT_VALUES_EQUAL(
            DefaultBlockSize * 1070,
            counters.ZeroBlocks.RequestBytes
        );
    }

    Y_UNIT_TEST(ShouldLocalReadWrite)
    {
        TTestBasicRuntime runtime;

        TTestEnv env(runtime);

        TPartitionClient client(runtime, env.ActorId);

        const TBlockRange64 diskRange(0, 5119);

        const TBlockRange64 blockRange1(1024, 4095);
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

            for (const auto& block: blocks) {
                for (auto c: block) {
                    UNIT_ASSERT_VALUES_EQUAL('A', c);
                }
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
                const auto& block = blocks[i];
                for (auto c: block) {
                    UNIT_ASSERT_VALUES_EQUAL('B', c);
                }
            }

            for (ui32 i = 120; i < blockRange3.Size(); ++i) {
                const auto& block = blocks[i];
                for (auto c: block) {
                    UNIT_ASSERT_VALUES_EQUAL(0, c);
                }
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
                blockRange1.Size() + blockRange3.Intersect(diskRange).Size()
            ),
            counters.ReadBlocks.RequestBytes
        );
        UNIT_ASSERT_VALUES_EQUAL(2, counters.WriteBlocks.Count);
        UNIT_ASSERT_VALUES_EQUAL(
            DefaultBlockSize * (
                blockRange1.Size() + blockRange2.Intersect(diskRange).Size()
            ),
            counters.WriteBlocks.RequestBytes
        );
    }

    Y_UNIT_TEST(ShouldWriteLargeBuffer)
    {
        TTestBasicRuntime runtime;

        TTestEnv env(runtime);

        TPartitionClient client(runtime, env.ActorId);

        client.WriteBlocks(TBlockRange64(1024, 4095), 1, 2048);

        {
            auto response = client.ReadBlocks(TBlockRange64(0, 5119));
            const auto& blocks = response->Record.GetBlocks();
            UNIT_ASSERT_VALUES_EQUAL(5120, blocks.BuffersSize());

            UNIT_ASSERT_VALUES_EQUAL(
                TString(DefaultBlockSize, 0),
                blocks.GetBuffers(0)
            );
            UNIT_ASSERT_VALUES_EQUAL(
                TString(DefaultBlockSize, 0),
                blocks.GetBuffers(1023)
            );
            UNIT_ASSERT_VALUES_EQUAL(
                TString(DefaultBlockSize, 1),
                blocks.GetBuffers(1024)
            );
            UNIT_ASSERT_VALUES_EQUAL(
                TString(DefaultBlockSize, 1),
                blocks.GetBuffers(4095)
            );
            UNIT_ASSERT_VALUES_EQUAL(
                TString(DefaultBlockSize, 0),
                blocks.GetBuffers(4096)
            );
            UNIT_ASSERT_VALUES_EQUAL(
                TString(DefaultBlockSize, 0),
                blocks.GetBuffers(5119)
            );
        }

        client.SendRequest(
            env.ActorId,
            std::make_unique<TEvNonreplPartitionPrivate::TEvUpdateCounters>()
        );

        runtime.DispatchEvents({}, TDuration::Seconds(1));

        auto& counters = env.StorageStatsServiceState->Counters.RequestCounters;
        UNIT_ASSERT_VALUES_EQUAL(1, counters.WriteBlocks.Count);
        UNIT_ASSERT_VALUES_EQUAL(
            DefaultBlockSize * 3072,
            counters.WriteBlocks.RequestBytes);
    }

    Y_UNIT_TEST(ShouldReadWriteZeroWithMonsterDisk)
    {
        TTestBasicRuntime runtime;

        TDevices devices;
        const ui64 blocksPerDevice = 93_GB / DefaultBlockSize;
        for (ui32 i = 0; i < 1024; ++i) {
            TTestEnv::AddDevice(
                runtime.GetNodeId(0),
                blocksPerDevice,
                Sprintf("vasya%u", i),
                devices
            );
        }

        TTestEnv env(runtime, NProto::VOLUME_IO_OK, std::move(devices));

        TPartitionClient client(runtime, env.ActorId);

        auto range1 = TBlockRange64::WithLength(0, 1024);
        auto range2 = TBlockRange64::WithLength(blocksPerDevice * 511, 1024);
        auto range3 = TBlockRange64::WithLength(blocksPerDevice * 1023, 1024);

#define TEST_READ(range, data) {                                               \
            auto response = client.ReadBlocks(range);                          \
            const auto& blocks = response->Record.GetBlocks();                 \
                                                                               \
            UNIT_ASSERT_VALUES_EQUAL(1024, blocks.BuffersSize());              \
            UNIT_ASSERT_VALUES_EQUAL(                                          \
                DefaultBlockSize,                                              \
                blocks.GetBuffers(0).size()                                    \
            );                                                                 \
            UNIT_ASSERT_VALUES_EQUAL(                                          \
                data,                                                          \
                blocks.GetBuffers(0)                                           \
            );                                                                 \
                                                                               \
            UNIT_ASSERT_VALUES_EQUAL(                                          \
                DefaultBlockSize,                                              \
                blocks.GetBuffers(1023).size()                                 \
            );                                                                 \
            UNIT_ASSERT_VALUES_EQUAL(                                          \
                data,                                                          \
                blocks.GetBuffers(1023)                                        \
            );                                                                 \
        }

        TEST_READ(range1, TString(DefaultBlockSize, 0));
        TEST_READ(range2, TString(DefaultBlockSize, 0));
        TEST_READ(range3, TString(DefaultBlockSize, 0));

        client.WriteBlocks(range1, 1);
        client.WriteBlocks(range2, 2);
        client.WriteBlocks(range3, 3);

        TEST_READ(range1, TString(DefaultBlockSize, 1));
        TEST_READ(range2, TString(DefaultBlockSize, 2));
        TEST_READ(range3, TString(DefaultBlockSize, 3));

        client.ZeroBlocks(range2);

        TEST_READ(range1, TString(DefaultBlockSize, 1));
        TEST_READ(range2, TString(DefaultBlockSize, 0));
        TEST_READ(range3, TString(DefaultBlockSize, 3));
    }

    Y_UNIT_TEST(ShouldHandleUndeliveredIO)
    {
        TTestBasicRuntime runtime;

        runtime.SetRegistrationObserverFunc(
            [] (auto& runtime, const auto& parentId, const auto& actorId)
        {
            Y_UNUSED(parentId);
            runtime.EnableScheduleForActor(actorId);
        });

        TTestEnv env(runtime);

        TPartitionClient client(runtime, env.ActorId);

        env.KillDiskAgent();

        {
            client.SendReadBlocksRequest(TBlockRange64(1024, 4095));
            runtime.AdvanceCurrentTime(TDuration::Seconds(1));
            runtime.DispatchEvents({}, TDuration::MilliSeconds(10));

            auto response = client.RecvReadBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_TIMEOUT, response->GetStatus());
            UNIT_ASSERT(response->GetErrorReason().Contains("request timed out"));
        }

        {
            client.SendWriteBlocksRequest(TBlockRange64(1024, 4095), 1);
            runtime.AdvanceCurrentTime(TDuration::Seconds(1));
            runtime.DispatchEvents({}, TDuration::MilliSeconds(10));
            auto response = client.RecvWriteBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_TIMEOUT, response->GetStatus());
            UNIT_ASSERT(response->GetErrorReason().Contains("request timed out"));
        }

        {
            client.SendZeroBlocksRequest(TBlockRange64(1024, 4095));
            runtime.AdvanceCurrentTime(TDuration::Seconds(1));
            runtime.DispatchEvents({}, TDuration::MilliSeconds(10));
            auto response = client.RecvZeroBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_TIMEOUT, response->GetStatus());
            UNIT_ASSERT(response->GetErrorReason().Contains("request timed out"));
        }
    }

    Y_UNIT_TEST(ShouldHandleTimedoutIO)
    {
        TTestBasicRuntime runtime;

        runtime.SetRegistrationObserverFunc(
            [] (auto& runtime, const auto& parentId, const auto& actorId)
        {
            Y_UNUSED(parentId);
            runtime.EnableScheduleForActor(actorId);
        });

        TTestEnv env(runtime);
        env.DiskAgentState->ResponseDelay = TDuration::Max();

        TPartitionClient client(runtime, env.ActorId);

        // timeout = 1s
        {
            client.SendReadBlocksRequest(TBlockRange64(1024, 4095));
            runtime.AdvanceCurrentTime(TDuration::Minutes(10));
            runtime.DispatchEvents();
            auto response = client.RecvReadBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_TIMEOUT, response->GetStatus());
            UNIT_ASSERT(response->GetErrorReason().Contains("timed out"));
        }

        // cumulative = 1s
        // timeout = 2s
        {
            client.SendWriteBlocksRequest(TBlockRange64(1024, 4095), 1);
            runtime.AdvanceCurrentTime(TDuration::Minutes(10));
            runtime.DispatchEvents();
            auto response = client.RecvWriteBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_TIMEOUT, response->GetStatus());
            UNIT_ASSERT(response->GetErrorReason().Contains("timed out"));
        }

        // cumulative = 3s
        // timeout = 4s
        {
            client.SendZeroBlocksRequest(TBlockRange64(1024, 4095));
            runtime.AdvanceCurrentTime(TDuration::Minutes(10));
            runtime.DispatchEvents();
            auto response = client.RecvZeroBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_TIMEOUT, response->GetStatus());
            UNIT_ASSERT(response->GetErrorReason().Contains("timed out"));
        }

        // cumulative = 7s
        // timeout = 5s
        {
            client.SendReadBlocksRequest(TBlockRange64(1024, 4095));
            runtime.AdvanceCurrentTime(TDuration::Minutes(10));
            runtime.DispatchEvents();
            auto response = client.RecvReadBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_TIMEOUT, response->GetStatus());
        }

        // cumulative = 12s
        // timeout = 5s
        {
            client.SendReadBlocksRequest(TBlockRange64(1024, 4095));
            runtime.AdvanceCurrentTime(TDuration::Minutes(10));
            runtime.DispatchEvents();
            auto response = client.RecvReadBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_TIMEOUT, response->GetStatus());
        }

        // cumulative = 17s
        // timeout = 5s
        {
            client.SendReadBlocksRequest(TBlockRange64(1024, 4095));
            runtime.AdvanceCurrentTime(TDuration::Minutes(10));
            runtime.DispatchEvents();
            auto response = client.RecvReadBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_TIMEOUT, response->GetStatus());
        }

        // the following attempts should get E_IO
        // cumulative = 22s
        // timeout = 5s
        {
            client.SendReadBlocksRequest(TBlockRange64(1024, 4095));
            runtime.DispatchEvents();
            auto response = client.RecvReadBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_IO, response->GetStatus());
        }

        {
            client.SendWriteBlocksRequest(TBlockRange64(1024, 4095), 1);
            runtime.DispatchEvents();
            auto response = client.RecvWriteBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_IO, response->GetStatus());
        }

        {
            client.SendZeroBlocksRequest(TBlockRange64(1024, 4095));
            runtime.DispatchEvents();
            auto response = client.RecvZeroBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_IO, response->GetStatus());
        }
    }

    Y_UNIT_TEST(ShouldRecoverFromShortTimeoutStreak)
    {
        TTestBasicRuntime runtime;

        runtime.SetRegistrationObserverFunc(
            [] (auto& runtime, const auto& parentId, const auto& actorId)
        {
            Y_UNUSED(parentId);
            runtime.EnableScheduleForActor(actorId);
        });

        TTestEnv env(runtime);
        env.DiskAgentState->ResponseDelay = TDuration::Max();

        TPartitionClient client(runtime, env.ActorId);

        // timeout = 1s
        {
            client.SendReadBlocksRequest(TBlockRange64(1024, 4095));
            runtime.AdvanceCurrentTime(TDuration::Minutes(10));
            runtime.DispatchEvents();
            auto response = client.RecvReadBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_TIMEOUT, response->GetStatus());
            UNIT_ASSERT(response->GetErrorReason().Contains("timed out"));
        }

        // cumulative = 1s
        // timeout = 2s
        {
            client.SendWriteBlocksRequest(TBlockRange64(1024, 4095), 1);
            runtime.AdvanceCurrentTime(TDuration::Minutes(10));
            runtime.DispatchEvents();
            auto response = client.RecvWriteBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_TIMEOUT, response->GetStatus());
            UNIT_ASSERT(response->GetErrorReason().Contains("timed out"));
        }

        // cumulative = 3s
        // timeout = 4s
        {
            client.SendZeroBlocksRequest(TBlockRange64(1024, 4095));
            runtime.AdvanceCurrentTime(TDuration::Minutes(10));
            runtime.DispatchEvents();
            auto response = client.RecvZeroBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_TIMEOUT, response->GetStatus());
            UNIT_ASSERT(response->GetErrorReason().Contains("timed out"));
        }

        // cumulative = 7s
        // timeout = 5s
        {
            client.SendReadBlocksRequest(TBlockRange64(1024, 4095));
            runtime.AdvanceCurrentTime(TDuration::Minutes(10));
            runtime.DispatchEvents();
            auto response = client.RecvReadBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_TIMEOUT, response->GetStatus());
        }

        // cumulative = 12s
        // timeout = 5s
        {
            client.SendReadBlocksRequest(TBlockRange64(1024, 4095));
            runtime.AdvanceCurrentTime(TDuration::Minutes(10));
            runtime.DispatchEvents();
            auto response = client.RecvReadBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_TIMEOUT, response->GetStatus());
        }

        env.DiskAgentState->ResponseDelay = TDuration::Zero();
        // cumulative = 17s
        // timeout = 0s
        {
            client.SendReadBlocksRequest(TBlockRange64(1024, 4095));
            runtime.DispatchEvents();
            auto response = client.RecvReadBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(S_OK, response->GetStatus());
        }
        env.DiskAgentState->ResponseDelay = TDuration::Max();

        // timeout = 1s
        {
            client.SendReadBlocksRequest(TBlockRange64(1024, 4095));
            runtime.AdvanceCurrentTime(TDuration::Minutes(10));
            runtime.DispatchEvents();
            auto response = client.RecvReadBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_TIMEOUT, response->GetStatus());
            UNIT_ASSERT(response->GetErrorReason().Contains("timed out"));
        }

        // cumulative = 1s
        // timeout = 2s
        {
            client.SendWriteBlocksRequest(TBlockRange64(1024, 4095), 1);
            runtime.AdvanceCurrentTime(TDuration::Minutes(10));
            runtime.DispatchEvents();
            auto response = client.RecvWriteBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_TIMEOUT, response->GetStatus());
            UNIT_ASSERT(response->GetErrorReason().Contains("timed out"));
        }

        // cumulative = 3s
        // timeout = 4s
        {
            client.SendZeroBlocksRequest(TBlockRange64(1024, 4095));
            runtime.AdvanceCurrentTime(TDuration::Minutes(10));
            runtime.DispatchEvents();
            auto response = client.RecvZeroBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_TIMEOUT, response->GetStatus());
            UNIT_ASSERT(response->GetErrorReason().Contains("timed out"));
        }

        // cumulative = 7s
        // timeout = 5s
        {
            client.SendReadBlocksRequest(TBlockRange64(1024, 4095));
            runtime.AdvanceCurrentTime(TDuration::Minutes(10));
            runtime.DispatchEvents();
            auto response = client.RecvReadBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_TIMEOUT, response->GetStatus());
        }

        // cumulative = 12s
        // timeout = 5s
        {
            client.SendReadBlocksRequest(TBlockRange64(1024, 4095));
            runtime.AdvanceCurrentTime(TDuration::Minutes(10));
            runtime.DispatchEvents();
            auto response = client.RecvReadBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_TIMEOUT, response->GetStatus());
        }

        // cumulative = 17s
        // timeout = 5s
        {
            client.SendReadBlocksRequest(TBlockRange64(1024, 4095));
            runtime.AdvanceCurrentTime(TDuration::Minutes(10));
            runtime.DispatchEvents();
            auto response = client.RecvReadBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_TIMEOUT, response->GetStatus());
        }

        // the following attempts should get E_IO
        // cumulative = 22s
        // timeout = 5s
        {
            client.SendReadBlocksRequest(TBlockRange64(1024, 4095));
            runtime.DispatchEvents();
            auto response = client.RecvReadBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_IO, response->GetStatus());
        }
    }

    Y_UNIT_TEST(ShouldLimitIO)
    {
        TTestBasicRuntime runtime;

        TTestEnv env(runtime);
        env.DiskAgentState->ResponseDelay = TDuration::Max();

        TPartitionClient client(runtime, env.ActorId);

        for (int i = 0; i != 1024; ++i) {
            client.SendReadBlocksRequest(TBlockRange64(1024, 4095));
        }

        {
            client.SendReadBlocksRequest(TBlockRange64(1024, 4095));
            runtime.DispatchEvents();
            auto response = client.RecvReadBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_REJECTED, response->GetStatus());
            UNIT_ASSERT_VALUES_EQUAL(response->GetErrorReason(), "Inflight limit reached");
        }

        {
            client.SendWriteBlocksRequest(TBlockRange64(1024, 4095), 1);
            runtime.DispatchEvents();
            auto response = client.RecvWriteBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_REJECTED, response->GetStatus());
            UNIT_ASSERT_VALUES_EQUAL(response->GetErrorReason(), "Inflight limit reached");
        }

        {
            client.SendZeroBlocksRequest(TBlockRange64(1024, 4095));
            runtime.DispatchEvents();
            auto response = client.RecvZeroBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_REJECTED, response->GetStatus());
            UNIT_ASSERT_VALUES_EQUAL(response->GetErrorReason(), "Inflight limit reached");
        }
    }

    Y_UNIT_TEST(ShouldHandleInvalidSessionError)
    {
        TTestBasicRuntime runtime;

        TTestEnv env(runtime);

        TPartitionClient client(runtime, env.ActorId);

        TActorId reacquireDiskRecipient;
        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvDiskAgent::EvReadDeviceBlocksRequest: {
                        auto response = std::make_unique<TEvDiskAgent::TEvReadDeviceBlocksResponse>(
                            MakeError(E_BS_INVALID_SESSION, "invalid session")
                        );

                        runtime.Send(new IEventHandle(
                            event->Sender,
                            event->Recipient,
                            response.release(),
                            0, // flags
                            event->Cookie
                        ), 0);

                        return TTestActorRuntime::EEventAction::DROP;
                    }

                    case TEvDiskAgent::EvWriteDeviceBlocksRequest: {
                        auto response = std::make_unique<TEvDiskAgent::TEvWriteDeviceBlocksResponse>(
                            MakeError(E_BS_INVALID_SESSION, "invalid session")
                        );

                        runtime.Send(new IEventHandle(
                            event->Sender,
                            event->Recipient,
                            response.release(),
                            0, // flags
                            event->Cookie
                        ), 0);

                        return TTestActorRuntime::EEventAction::DROP;
                    }

                    case TEvDiskAgent::EvZeroDeviceBlocksRequest: {
                        auto response = std::make_unique<TEvDiskAgent::TEvZeroDeviceBlocksResponse>(
                            MakeError(E_BS_INVALID_SESSION, "invalid session")
                        );

                        runtime.Send(new IEventHandle(
                            event->Sender,
                            event->Recipient,
                            response.release(),
                            0, // flags
                            event->Cookie
                        ), 0);

                        return TTestActorRuntime::EEventAction::DROP;
                    }

                    case TEvVolume::EvReacquireDisk: {
                        reacquireDiskRecipient = event->Recipient;

                        return TTestActorRuntime::EEventAction::DROP;
                    }
                }

                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            }
        );

        {
            client.SendReadBlocksRequest(TBlockRange64(0, 1023));
            auto response = client.RecvReadBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_REJECTED, response->GetStatus());
        }

        UNIT_ASSERT_VALUES_EQUAL(env.VolumeActorId, reacquireDiskRecipient);

        reacquireDiskRecipient = {};

        {
            client.SendWriteBlocksRequest(TBlockRange64(0, 1023), 1);
            auto response = client.RecvWriteBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_REJECTED, response->GetStatus());
        }

        UNIT_ASSERT_VALUES_EQUAL(env.VolumeActorId, reacquireDiskRecipient);

        reacquireDiskRecipient = {};

        {
            client.SendZeroBlocksRequest(TBlockRange64(0, 1023));
            auto response = client.RecvZeroBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_REJECTED, response->GetStatus());
        }

        UNIT_ASSERT_VALUES_EQUAL(env.VolumeActorId, reacquireDiskRecipient);
    }

    Y_UNIT_TEST(ShouldSupportReadOnlyMode)
    {
        TTestBasicRuntime runtime;

        TTestEnv env(runtime, NProto::VOLUME_IO_ERROR_READ_ONLY);

        TPartitionClient client(runtime, env.ActorId);

        TString expectedBlockData(DefaultBlockSize, 0);

        auto readBlocks = [&] {
            auto response = client.ReadBlocks(TBlockRange64(1024, 4095));
            const auto& blocks = response->Record.GetBlocks();

            UNIT_ASSERT_VALUES_EQUAL(3072, blocks.BuffersSize());
            UNIT_ASSERT_VALUES_EQUAL(DefaultBlockSize, blocks.GetBuffers(0).size());
            UNIT_ASSERT_VALUES_EQUAL(expectedBlockData, blocks.GetBuffers(0));

            UNIT_ASSERT_VALUES_EQUAL(DefaultBlockSize, blocks.GetBuffers(3071).size());
            UNIT_ASSERT_VALUES_EQUAL(expectedBlockData, blocks.GetBuffers(3071));
        };

        readBlocks();

        {
            client.SendWriteBlocksRequest(TBlockRange64(1024, 4095), 1);
            auto response = client.RecvWriteBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_IO, response->GetStatus());
        }
        {
            client.SendWriteBlocksRequest(TBlockRange64(1024, 4023), 2);
            auto response = client.RecvWriteBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_IO, response->GetStatus());
        }

        readBlocks();

        {
            client.SendZeroBlocksRequest(TBlockRange64(2024, 3023));
            auto response = client.RecvZeroBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_IO, response->GetStatus());
        }

        readBlocks();

        expectedBlockData = TString(DefaultBlockSize, 'A');
        {
            auto request = client.CreateWriteBlocksLocalRequest(
                TBlockRange64(1024, 4095),
                expectedBlockData);
            request->Record.MutableHeaders()->SetIsBackgroundRequest(true);
            client.SendRequest(client.GetActorId(), std::move(request));
            auto response = client.RecvWriteBlocksLocalResponse();
            UNIT_ASSERT_VALUES_EQUAL_C(
                S_OK,
                response->GetStatus(),
                response->GetErrorReason());
        }

        readBlocks();
    }

    Y_UNIT_TEST(ShouldSendStatsToVolume)
    {
        TTestBasicRuntime runtime;

        runtime.SetRegistrationObserverFunc(
            [] (auto& runtime, const auto& parentId, const auto& actorId)
        {
            Y_UNUSED(parentId);
            runtime.EnableScheduleForActor(actorId);
        });

        TTestEnv env(runtime);

        bool done = false;
        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvStatsService::EvVolumePartCounters:
                        if (event->Recipient == MakeStorageStatsServiceId()) {
                            done = true;
                        }
                        break;
                }

                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            }
        );

        TPartitionClient client(runtime, env.ActorId);

        {
            runtime.AdvanceCurrentTime(TDuration::Seconds(15));
            runtime.DispatchEvents({}, TDuration::Seconds(1));
            UNIT_ASSERT_VALUES_EQUAL(true, done);
        }
    }

    Y_UNIT_TEST(ShouldEnforceReadOnlyStateWithUnavailableAgent)
    {
        TTestBasicRuntime runtime;

        runtime.SetRegistrationObserverFunc(
            [] (auto& runtime, const auto& parentId, const auto& actorId)
        {
            Y_UNUSED(parentId);
            runtime.EnableScheduleForActor(actorId);
        });

        TTestEnv env(runtime);
        env.DiskAgentState->ResponseDelay = TDuration::Max();

        TPartitionClient client(runtime, env.ActorId);

        // wait for vasya
        for (;;) {
            client.SendReadBlocksRequest(TBlockRange64(0, 1024));
            runtime.AdvanceCurrentTime(TDuration::Minutes(10));
            runtime.DispatchEvents();
            auto response = client.RecvReadBlocksResponse();
            if (response->GetStatus() == E_IO) {
                break;
            }
            UNIT_ASSERT_VALUES_EQUAL(E_TIMEOUT, response->GetStatus());
            UNIT_ASSERT(response->GetErrorReason().Contains("timed out"));
        }

        // read from petya
        {
            client.SendReadBlocksRequest(TBlockRange64(2048, 3072));
            runtime.DispatchEvents();
            auto response = client.RecvReadBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_TIMEOUT, response->GetStatus());
            UNIT_ASSERT(response->GetErrorReason().Contains("timed out"));
        }

        // can't write to petya
        {
            client.SendWriteBlocksRequest(TBlockRange64(2048, 3072), 1);
            runtime.DispatchEvents();
            auto response = client.RecvWriteBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_IO, response->GetStatus());
            UNIT_ASSERT(response->GetErrorReason().Contains("disk has broken device"));
        }
    }
}

}   // namespace NCloud::NBlockStore::NStorage
