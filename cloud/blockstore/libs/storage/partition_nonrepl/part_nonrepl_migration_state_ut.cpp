#include "part_nonrepl_migration_state.h"

#include <cloud/blockstore/config/storage.pb.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/partition_nonrepl/config.h>
#include <cloud/blockstore/libs/storage/protos/disk.pb.h>

#include <cloud/storage/core/libs/common/error.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/datetime/base.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

namespace {

////////////////////////////////////////////////////////////////////////////////

struct TEnv
{
    TDevices Devices;
    TDevices DstDevices;
    TNonreplicatedPartitionConfigPtr Config;
    TNonreplicatedPartitionConfigPtr DstConfig;

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
            THashSet<TString>() // freshDeviceIds
        );

        {
            auto* device = DstDevices.Add();
            device->SetAgentId("3");
            device->SetBlocksCount(1024);
            device->SetDeviceUUID("3_1");
        }
        {
            auto* device = DstDevices.Add();
            device->SetAgentId("4");
            device->SetBlocksCount(1024);
            device->SetDeviceUUID("4_1");
        }
        {
            auto* device = DstDevices.Add();
            device->SetAgentId("5");
            device->SetBlocksCount(1024);
            device->SetDeviceUUID("5_1");
        }
        {
            auto* device = DstDevices.Add();
            device->SetBlocksCount(1024);
        }

        DstConfig = std::make_shared<TNonreplicatedPartitionConfig>(
            DstDevices,
            NProto::VOLUME_IO_OK,
            "vol0",
            4_KB,
            NActors::TActorId(),
            false, // muteIOErrors
            false, // markBlocksUsed
            THashSet<TString>() // freshDeviceIds
        );
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TNonreplicatedPartitionMigrationStateTest)
{
    Y_UNIT_TEST(ShouldCalculateMigrationTimeout)
    {
        TEnv env;

        TNonreplicatedPartitionMigrationState state(
            std::make_shared<TStorageConfig>(
                NProto::TStorageServiceConfig(),
                nullptr),
            0,          // initialMigrationIndex
            "xxx",      // rwClientId
            env.Config
        );

        state.SetupDstPartition(env.DstConfig);
        state.MarkMigrated(TBlockRange64(3072, 4096));
        state.SkipMigratedRanges();

        UNIT_ASSERT(state.IsMigrationStarted());

        UNIT_ASSERT_VALUES_EQUAL(
            TDuration::Seconds(1) / 2,
            state.CalculateMigrationTimeout(
                16,
                4
            )
        );

        UNIT_ASSERT(state.AdvanceMigrationIndex());
        UNIT_ASSERT(state.IsMigrationStarted());

        UNIT_ASSERT_VALUES_EQUAL(
            TDuration::Seconds(1) / 2,
            state.CalculateMigrationTimeout(
                16,
                4
            )
        );

        UNIT_ASSERT(state.AdvanceMigrationIndex());
        UNIT_ASSERT(state.IsMigrationStarted());

        UNIT_ASSERT_VALUES_EQUAL(
            TDuration::Seconds(1),
            state.CalculateMigrationTimeout(
                16,
                4
            )
        );

        UNIT_ASSERT(!state.AdvanceMigrationIndex());
        UNIT_ASSERT(!state.IsMigrationStarted());
    }

    Y_UNIT_TEST(ShouldProperlyAccountForMigratedPrefix)
    {
        TEnv env;

        TNonreplicatedPartitionMigrationState state(
            std::make_shared<TStorageConfig>(
                NProto::TStorageServiceConfig(),
                nullptr),
            0,          // initialMigrationIndex
            "xxx",      // rwClientId
            env.Config
        );

        state.SetupDstPartition(env.DstConfig);
        state.MarkMigrated(TBlockRange64(0, 1024));
        state.SkipMigratedRanges();

        UNIT_ASSERT(state.IsMigrationStarted());
        auto range = state.BuildMigrationRange();
        UNIT_ASSERT_VALUES_EQUAL(1025, range.Start);
        UNIT_ASSERT_VALUES_EQUAL(2048, range.End);

        UNIT_ASSERT(state.AdvanceMigrationIndex());
        UNIT_ASSERT(state.IsMigrationStarted());
        range = state.BuildMigrationRange();
        UNIT_ASSERT_VALUES_EQUAL(2049, range.Start);
        UNIT_ASSERT_VALUES_EQUAL(3072, range.End);

        UNIT_ASSERT(state.AdvanceMigrationIndex());
        UNIT_ASSERT(state.IsMigrationStarted());
        range = state.BuildMigrationRange();
        UNIT_ASSERT_VALUES_EQUAL(3073, range.Start);
        UNIT_ASSERT_VALUES_EQUAL(4095, range.End);

        UNIT_ASSERT(!state.AdvanceMigrationIndex());
        UNIT_ASSERT(!state.IsMigrationStarted());
    }
}

}   // namespace NCloud::NBlockStore::NStorage
