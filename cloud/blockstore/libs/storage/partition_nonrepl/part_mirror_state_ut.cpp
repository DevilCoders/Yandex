#include "part_mirror_state.h"

#include <cloud/blockstore/config/storage.pb.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/partition_nonrepl/config.h>
#include <cloud/blockstore/libs/storage/protos/disk.pb.h>

#include <library/cpp/actors/core/actorid.h>
#include <library/cpp/testing/unittest/registar.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

namespace {

////////////////////////////////////////////////////////////////////////////////

struct TEnv
{
    TDevices Devices;
    TDevices ReplicaDevices;
    TNonreplicatedPartitionConfigPtr Config;

    TEnv()
    {
        {
            auto* device = Devices.Add();
            device->SetAgentId("1");
            device->SetBlocksCount(1024);
            device->SetDeviceUUID("1_1");
        }
        {
            auto* device = Devices.Add();
            device->SetAgentId("1");
            device->SetBlocksCount(1024);
            device->SetDeviceUUID("1_2");
        }
        {
            auto* device = Devices.Add();
            device->SetAgentId("2");
            device->SetBlocksCount(1024);
            device->SetDeviceUUID("2_1");
        }
        {
            auto* device = Devices.Add();
            device->SetAgentId("1");
            device->SetBlocksCount(1024);
            device->SetDeviceUUID("1_3");
        }

        Config = std::make_shared<TNonreplicatedPartitionConfig>(
            Devices,
            NProto::VOLUME_IO_OK,
            "vol0",
            4_KB,
            NActors::TActorId(),
            false, // muteIOErrors
            false, // markBlocksUsed
            THashSet<TString>{"1_2", "2_1", "3_1"}
        );

        {
            auto* device = ReplicaDevices.Add();
            device->SetAgentId("3");
            device->SetBlocksCount(1024);
            device->SetDeviceUUID("3_1");
        }
        {
            auto* device = ReplicaDevices.Add();
            device->SetAgentId("4");
            device->SetBlocksCount(1024);
            device->SetDeviceUUID("4_1");
        }
        {
            auto* device = ReplicaDevices.Add();
            device->SetAgentId("5");
            device->SetBlocksCount(1024);
            device->SetDeviceUUID("5_1");
        }
        {
            auto* device = ReplicaDevices.Add();
            device->SetBlocksCount(1024);
        }
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TMirrorPartitionStateTest)
{
#define TEST_READ_REPLICA(expected, state, startIndex, blockCount) {           \
    NActors::TActorId actorId;                                                 \
    const auto blockRange = TBlockRange64::WithLength(startIndex, blockCount); \
    const auto error = state.NextReadReplica(blockRange, &actorId);            \
    UNIT_ASSERT_VALUES_EQUAL_C(S_OK, error.GetCode(), error.GetMessage());     \
    UNIT_ASSERT_VALUES_EQUAL(expected, actorId);                               \
}                                                                              \
// TEST_READ_REPLICA

    Y_UNIT_TEST(ShouldSelectProperReadReplicas)
    {
        TEnv env;

        TMirrorPartitionState state(
            std::make_shared<TStorageConfig>(
                NProto::TStorageServiceConfig(),
                nullptr),
            "xxx",      // rwClientId
            env.Config,
            {env.ReplicaDevices}
        );

        NActors::TActorId actor1(1, "vasya");
        NActors::TActorId actor2(2, "petya");

        state.AddReplicaActor(actor1);
        state.AddReplicaActor(actor2);

        TEST_READ_REPLICA(actor1, state, 0, 1024);
        TEST_READ_REPLICA(actor1, state, 0, 1024);

        TEST_READ_REPLICA(actor2, state, 1024, 1024);
        TEST_READ_REPLICA(actor2, state, 1024, 1024);

        TEST_READ_REPLICA(actor2, state, 2048, 1024);
        TEST_READ_REPLICA(actor2, state, 2048, 1024);

        TEST_READ_REPLICA(actor1, state, 3072, 1024);
        TEST_READ_REPLICA(actor1, state, 3072, 1024);
    }

    Y_UNIT_TEST(ShouldBuildMigrationConfig)
    {
        TEnv env;

        TMirrorPartitionState state(
            std::make_shared<TStorageConfig>(
                NProto::TStorageServiceConfig(),
                nullptr),
            "xxx",      // rwClientId
            env.Config,
            {env.ReplicaDevices}
        );

        auto error = state.PrepareMigrationConfig();
        UNIT_ASSERT_VALUES_EQUAL_C(S_OK, error.GetCode(), error.GetMessage());

        UNIT_ASSERT_VALUES_EQUAL(2, state.GetReplicaInfos().size());

        const auto& replica0 = state.GetReplicaInfos()[0];
        UNIT_ASSERT_VALUES_EQUAL(
            "1_1",
            replica0.Config->GetDevices()[0].GetDeviceUUID());
        UNIT_ASSERT_VALUES_EQUAL(
            1024,
            replica0.Config->GetDevices()[0].GetBlocksCount());
        UNIT_ASSERT_VALUES_EQUAL(
            "4_1",
            replica0.Config->GetDevices()[1].GetDeviceUUID());
        UNIT_ASSERT_VALUES_EQUAL(
            1024,
            replica0.Config->GetDevices()[1].GetBlocksCount());
        UNIT_ASSERT_VALUES_EQUAL(
            "2_1",
            replica0.Config->GetDevices()[2].GetDeviceUUID());
        UNIT_ASSERT_VALUES_EQUAL(
            1024,
            replica0.Config->GetDevices()[2].GetBlocksCount());
        UNIT_ASSERT_VALUES_EQUAL(
            "1_3",
            replica0.Config->GetDevices()[3].GetDeviceUUID());
        UNIT_ASSERT_VALUES_EQUAL(
            1024,
            replica0.Config->GetDevices()[3].GetBlocksCount());

        const auto& migrations0 = replica0.Migrations;
        UNIT_ASSERT_VALUES_EQUAL(1, migrations0.size());

        UNIT_ASSERT_VALUES_EQUAL("4_1", migrations0[0].GetSourceDeviceId());
        UNIT_ASSERT_VALUES_EQUAL(
            "1_2",
            migrations0[0].GetTargetDevice().GetDeviceUUID());
        UNIT_ASSERT_VALUES_EQUAL(
            1024,
            migrations0[0].GetTargetDevice().GetBlocksCount());

        const auto& replica1 = state.GetReplicaInfos()[1];
        UNIT_ASSERT_VALUES_EQUAL(
            "3_1",
            replica1.Config->GetDevices()[0].GetDeviceUUID());
        UNIT_ASSERT_VALUES_EQUAL(
            1024,
            replica1.Config->GetDevices()[0].GetBlocksCount());
        UNIT_ASSERT_VALUES_EQUAL(
            "",
            replica1.Config->GetDevices()[1].GetDeviceUUID());
        UNIT_ASSERT_VALUES_EQUAL(
            1024,
            replica1.Config->GetDevices()[1].GetBlocksCount());
        UNIT_ASSERT_VALUES_EQUAL(
            "5_1",
            replica1.Config->GetDevices()[2].GetDeviceUUID());
        UNIT_ASSERT_VALUES_EQUAL(
            1024,
            replica1.Config->GetDevices()[2].GetBlocksCount());
        UNIT_ASSERT_VALUES_EQUAL(
            "",
            replica1.Config->GetDevices()[3].GetDeviceUUID());
        UNIT_ASSERT_VALUES_EQUAL(
            1024,
            replica1.Config->GetDevices()[3].GetBlocksCount());

        const auto& migrations1 = replica1.Migrations;
        UNIT_ASSERT_VALUES_EQUAL(0, migrations1.size());
    }

    // TODO: test config validation / migration config preparation failures

#undef TEST_READ_REPLICA
}

}   // namespace NCloud::NBlockStore::NStorage
