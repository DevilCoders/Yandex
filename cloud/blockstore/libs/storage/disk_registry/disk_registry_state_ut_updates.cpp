#include "disk_registry_state.h"

#include "disk_registry_database.h"

#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/disk_registry/testlib/test_state.h>
#include <cloud/blockstore/libs/storage/testlib/test_executor.h>
#include <cloud/blockstore/libs/storage/testlib/ut_helpers.h>
#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/diagnostics/monitoring.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/guid.h>
#include <util/generic/size_literals.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NDiskRegistryStateTest;

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TDiskRegistryStateUpdatesTest)
{
    Y_UNIT_TEST(ShouldUpdateDiskBlockSize)
    {
        const TVector agents {
            AgentConfig(1, {
                Device("dev-1", "uuid-1.1", "", DefaultBlockSize, 10_GB),
            }),
        };

        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const auto diskId = "disk-1";

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents(agents)
            .WithDisks({
                Disk(diskId, { "uuid-1.1" })
            })
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            auto error = state.UpdateDiskBlockSize(db, "", 1_KB, false);
            UNIT_ASSERT_VALUES_EQUAL_C(E_ARGUMENT, error.GetCode(),
                "should reject a request with empty disk id");

            error = state.UpdateDiskBlockSize(db, "nonexistent", 1_KB, false);
            UNIT_ASSERT_VALUES_EQUAL_C(E_NOT_FOUND, error.GetCode(),
                "should reject a request with nonexistent disk id");

            error = state.UpdateDiskBlockSize(db, diskId, 1_KB, false);
            UNIT_ASSERT_VALUES_EQUAL_C(E_ARGUMENT, error.GetCode(),
                "should reject a request with too small block size");

            error = state.UpdateDiskBlockSize(db, diskId, 256_KB, false);
            UNIT_ASSERT_VALUES_EQUAL_C(E_ARGUMENT, error.GetCode(),
                "should reject a request with too big block size");

            error = state.UpdateDiskBlockSize(db, diskId, 64_KB, false);
            UNIT_ASSERT_VALUES_EQUAL_C(S_OK, error.GetCode(),
                "should complete a request with good block size");

            error = state.UpdateDiskBlockSize(db, diskId, 64_KB, false);
            UNIT_ASSERT_VALUES_EQUAL_C(S_FALSE, error.GetCode(),
                "should complete a request with S_FALSE to reset the "
                "exact same block size as it is right now");

            error = state.UpdateDiskBlockSize(db, diskId, 1_KB, true);
            UNIT_ASSERT_VALUES_EQUAL_C(S_OK, error.GetCode(),
                "should complete a request with too small block size "
                "if force flag is given");
        });
    }

    Y_UNIT_TEST(ShouldUpdateDiskBlockSizeWhenDeviceResizesBadly)
    {
        const auto deviceId = "uuid-1.1";
        const auto diskId = "disk-1";

        const TVector agents {
            AgentConfig(1, {
                Device("dev-1", deviceId, "", DefaultBlockSize, 10000000000),
            })
        };

        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents(agents)
            .WithDisks({
                Disk(diskId, { deviceId })
            })
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            auto error = state.UpdateDiskBlockSize(db, diskId, 128_KB, false);
            UNIT_ASSERT_VALUES_EQUAL_C(E_ARGUMENT, error.GetCode(),
                "should reject a request where device will be badly resized");

            const auto blocksCountBeforeUpdate = state.GetDevice(deviceId)
                .GetBlocksCount();

            error = state.UpdateDiskBlockSize(db, diskId, 128_KB, true);
            UNIT_ASSERT_VALUES_EQUAL_C(S_OK, error.GetCode(),
                "should complete a request where device will be badly resized "
                "with force");
            UNIT_ASSERT_VALUES_UNEQUAL_C(blocksCountBeforeUpdate,
                state.GetDevice(deviceId).GetBlocksCount(),
                "should update blocks count for badly resized disk"
            );
        });
    }
}

}   // namespace NCloud::NBlockStore::NStorage
