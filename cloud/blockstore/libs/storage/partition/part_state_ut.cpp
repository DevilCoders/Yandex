#include "part_state.h"

#include <cloud/blockstore/libs/storage/model/channel_data_kind.h>
#include <cloud/blockstore/libs/storage/partition/part_schema.h>
#include <cloud/blockstore/libs/storage/testlib/test_executor.h>
#include <cloud/blockstore/libs/storage/testlib/ut_helpers.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/size_literals.h>

namespace NCloud::NBlockStore::NStorage::NPartition {

namespace {

////////////////////////////////////////////////////////////////////////////////

const ui32 DefaultBlockCount = 1000;
const ui32 DataChannelStart = 3;

////////////////////////////////////////////////////////////////////////////////

NProto::TPartitionMeta DefaultConfig(size_t channelCount, size_t blockCount)
{
    NProto::TPartitionMeta meta;

    auto& config = *meta.MutableConfig();
    config.SetBlockSize(DefaultBlockSize);
    config.SetBlocksCount(blockCount);

    auto cps = config.MutableExplicitChannelProfiles();
    cps->Add()->SetDataKind(static_cast<ui32>(EChannelDataKind::System));
    cps->Add()->SetDataKind(static_cast<ui32>(EChannelDataKind::Log));
    cps->Add()->SetDataKind(static_cast<ui32>(EChannelDataKind::Index));

    for (ui32 i = 0; i < channelCount; ++i) {
        cps->Add()->SetDataKind(static_cast<ui32>(EChannelDataKind::Merged));
    }

    cps->Add()->SetDataKind(static_cast<ui32>(EChannelDataKind::Fresh));

    return meta;
}

TBackpressureFeaturesConfig DefaultBPConfig()
{
    return {
        {
            30,     // compaction score limit
            10,     // compaction score threshold
            10,     // compaction score feature max value
        },
        {
            1600_KB,// fresh byte count limit
            400_KB, // fresh byte count threshold
            10,     // fresh byte count max value
        },
    };
}

TFreeSpaceConfig DefaultFreeSpaceConfig()
{
    return {
        0.25,   // free space threshold
        0.15,   // min free space
    };
}

////////////////////////////////////////////////////////////////////////////////

struct TNoBackpressurePolicy
    : ICompactionPolicy
{
    float CalculateScore(const TRangeStat& stat) const override
    {
        return stat.BlobCount;
    }

    bool BackpressureEnabled() const override
    {
        return false;
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TPartitionStateTest)
{

#define CHECK_DISK_SPACE_SCORE(expected)                                \
            UNIT_ASSERT_DOUBLES_EQUAL(                                  \
                expected,                                               \
                state.CalculateCurrentBackpressure().DiskSpaceScore,    \
                1e-5                                                    \
            )                                                           \
// CHECK_DISK_SPACE_SCORE

    Y_UNIT_TEST(UpdatePermissions)
    {
        const size_t maxChannelCount = 3;

        for (size_t channelCount = 1; channelCount <= maxChannelCount; ++channelCount) {
            TPartitionState state(
                DefaultConfig(channelCount, DefaultBlockCount),
                0,  // generation
                BuildDefaultCompactionPolicy(5),
                0,  // compactionScoreHistorySize
                0,  // cleanupScoreHistorySize
                DefaultBPConfig(),
                DefaultFreeSpaceConfig(),
                Max(),  // maxIORequestsInFlight
                0,  // lastCommitId
                channelCount + 4,  // channelCount
                0,  // mixedIndexCacheSize
                false  // calculateLogicalUsedBlocks
            );

            UNIT_ASSERT(state.IsCompactionAllowed());

            ui32 channelId = 0;
            const double baseScore = 1;
            const double maxScore = 100;

            while (channelId < 3) {
                UNIT_ASSERT(!state.UpdatePermissions(
                    channelId,
                    EChannelPermission::SystemWritesAllowed
                    | EChannelPermission::UserWritesAllowed
                ));
                UNIT_ASSERT(state.IsCompactionAllowed());
                CHECK_DISK_SPACE_SCORE(baseScore);

                UNIT_ASSERT(
                    !state.UpdatePermissions(
                        channelId, EChannelPermission::SystemWritesAllowed
                    )
                );
                UNIT_ASSERT(state.IsCompactionAllowed());
                CHECK_DISK_SPACE_SCORE(baseScore);

                UNIT_ASSERT(state.UpdatePermissions(
                    channelId, EChannelPermission::UserWritesAllowed
                ));
                UNIT_ASSERT(!state.IsCompactionAllowed());
                CHECK_DISK_SPACE_SCORE(maxScore);

                UNIT_ASSERT(!state.UpdatePermissions(channelId, {}));
                UNIT_ASSERT(!state.IsCompactionAllowed());
                CHECK_DISK_SPACE_SCORE(maxScore);

                UNIT_ASSERT(state.UpdatePermissions(
                    channelId,
                    EChannelPermission::SystemWritesAllowed
                    | EChannelPermission::UserWritesAllowed
                ));
                CHECK_DISK_SPACE_SCORE(baseScore);

                ++channelId;
            }

            const TVector<TVector<double>> expectedScores{
                {baseScore, maxScore},
                {baseScore, 2, maxScore},
                {baseScore, 1.5, 3, maxScore},
            };
            while (channelId <= 2 + channelCount) {
                const auto currentScore =
                    expectedScores[channelCount - 1][channelId - 3];
                const auto nextScore =
                    expectedScores[channelCount - 1][channelId - 3 + 1];

                UNIT_ASSERT(!state.UpdatePermissions(
                    channelId,
                    EChannelPermission::SystemWritesAllowed
                    | EChannelPermission::UserWritesAllowed
                ));
                UNIT_ASSERT(state.IsCompactionAllowed());
                CHECK_DISK_SPACE_SCORE(currentScore);

                UNIT_ASSERT(state.UpdatePermissions(
                    channelId, EChannelPermission::SystemWritesAllowed
                ));
                UNIT_ASSERT(state.IsCompactionAllowed());
                CHECK_DISK_SPACE_SCORE(nextScore);

                UNIT_ASSERT(state.UpdatePermissions(
                    channelId, EChannelPermission::UserWritesAllowed
                ));
                UNIT_ASSERT_VALUES_EQUAL(
                    channelId < 2 + channelCount, state.IsCompactionAllowed()
                );
                CHECK_DISK_SPACE_SCORE(currentScore);

                UNIT_ASSERT(state.UpdatePermissions(channelId, {}));
                UNIT_ASSERT_VALUES_EQUAL(
                    channelId < 2 + channelCount, state.IsCompactionAllowed()
                );
                CHECK_DISK_SPACE_SCORE(nextScore);

                ++channelId;
            }

            // TODO: fresh channels
        }
    }

    Y_UNIT_TEST(TestReassignedChannelsCollection)
    {
        TPartitionState state(
            DefaultConfig(2, DefaultBlockCount),
            0,  // generation
            BuildDefaultCompactionPolicy(5),
            0,  // compactionScoreHistorySize
            0,  // cleanupScoreHistorySize
            DefaultBPConfig(),
            DefaultFreeSpaceConfig(),
            Max(),  // maxIORequestsInFlight
            0,  // lastCommitId
            6,  // channelCount
            0,  // mixedIndexCacheSize
            false  // calculateLogicalUsedBlocks
        );

        UNIT_ASSERT(
            state.IsWriteAllowed(EChannelPermission::UserWritesAllowed)
        );
        UNIT_ASSERT_VALUES_EQUAL(0, state.GetChannelsToReassign().size());

        state.UpdatePermissions(
            DataChannelStart,
            EChannelPermission::SystemWritesAllowed
        );

        UNIT_ASSERT(
            state.IsWriteAllowed(EChannelPermission::UserWritesAllowed)
        );
        auto channelsToReassign = state.GetChannelsToReassign();
        UNIT_ASSERT_VALUES_EQUAL(1, channelsToReassign.size());
        UNIT_ASSERT_VALUES_EQUAL(DataChannelStart, channelsToReassign[0]);

        state.UpdatePermissions(
            DataChannelStart + 1,
            EChannelPermission::SystemWritesAllowed
        );

        UNIT_ASSERT(
            !state.IsWriteAllowed(EChannelPermission::UserWritesAllowed)
        );
        channelsToReassign = state.GetChannelsToReassign();
        UNIT_ASSERT_VALUES_EQUAL(2, channelsToReassign.size());
        UNIT_ASSERT_VALUES_EQUAL(DataChannelStart, channelsToReassign[0]);
        UNIT_ASSERT_VALUES_EQUAL(DataChannelStart + 1, channelsToReassign[1]);
    }

    Y_UNIT_TEST(UpdateChannelFreeSpaceShare)
    {
        const size_t maxChannelCount = 3;

        for (size_t channelCount = 1; channelCount <= maxChannelCount; ++channelCount) {
            TPartitionState state(
                DefaultConfig(channelCount, DefaultBlockCount),
                0,  // generation
                BuildDefaultCompactionPolicy(5),
                0,  // compactionScoreHistorySize
                0,  // cleanupScoreHistorySize
                DefaultBPConfig(),
                DefaultFreeSpaceConfig(),
                Max(),  // maxIORequestsInFlight
                0,  // lastCommitId
                channelCount + 4,  // channelCount
                0,  // mixedIndexCacheSize
                false  // calculateLogicalUsedBlocks
            );

            const double baseScore = 1;
            const double maxScore = 100;

            for (ui32 ch = 0; ch < state.GetChannelCount(); ++ch) {
                const auto kind = state.GetChannelDataKind(ch);
                switch (kind) {
                    case EChannelDataKind::System:
                    case EChannelDataKind::Log:
                    case EChannelDataKind::Index: {
                        UNIT_ASSERT(!state.UpdateChannelFreeSpaceShare(ch, 0));
                        UNIT_ASSERT(!state.UpdateChannelFreeSpaceShare(ch, 0.25));
                        CHECK_DISK_SPACE_SCORE(baseScore);

                        UNIT_ASSERT(state.UpdateChannelFreeSpaceShare(ch, 0.20));
                        CHECK_DISK_SPACE_SCORE(2);

                        UNIT_ASSERT(state.UpdateChannelFreeSpaceShare(ch, 0.15));
                        CHECK_DISK_SPACE_SCORE(maxScore);

                        UNIT_ASSERT(!state.UpdateChannelFreeSpaceShare(ch, 0));
                        UNIT_ASSERT(state.UpdateChannelFreeSpaceShare(ch, 1));

                        CHECK_DISK_SPACE_SCORE(baseScore);

                        break;
                    }
                    case EChannelDataKind::Mixed:
                    case EChannelDataKind::Merged: {
                        UNIT_ASSERT(state.UpdateChannelFreeSpaceShare(ch, 0.2));

                        constexpr ui32 FirstDataChannel = 3;
                        CHECK_DISK_SPACE_SCORE(
                            1 / (1 - 0.5 * (ch - FirstDataChannel + 1) / channelCount)
                        );
                        break;
                    }

                    case EChannelDataKind::Fresh: {
                        UNIT_ASSERT(state.UpdateChannelFreeSpaceShare(ch, 0.2));
                        CHECK_DISK_SPACE_SCORE(maxScore);
                        break;
                    }

                    default: {
                        Y_FAIL("unsupported kind: %u", static_cast<ui32>(kind));
                    }
                }
            }
        }
    }

    Y_UNIT_TEST(PickProperNextChannel)
    {
        for (auto kind: {EChannelDataKind::Mixed, EChannelDataKind::Merged}) {
            TPartitionState state(
                DefaultConfig(MaxChannelsCount, DefaultBlockCount),
                0,  // generation
                BuildDefaultCompactionPolicy(5),
                0,  // compactionScoreHistorySize
                0,  // cleanupScoreHistorySize
                DefaultBPConfig(),
                DefaultFreeSpaceConfig(),
                Max(),  // maxIORequestsInFlight
                0,  // lastCommitId
                MaxChannelsCount + 4,  // channelCount
                0,  // mixedIndexCacheSize
                false  // calculateLogicalUsedBlocks
            );
            auto blobId = state.GenerateBlobId(
                kind,
                EChannelPermission::UserWritesAllowed,
                1,
                1024);
            UNIT_ASSERT(blobId.Channel() == TPartitionSchema::FirstDataChannel);
        }
    }

    Y_UNIT_TEST(PickProperNextChannelWithExplicitChannelProfiles)
    {
        NProto::TPartitionMeta meta;
        auto& config = *meta.MutableConfig();
        config.SetBlockSize(DefaultBlockSize);
        config.SetBlocksCount(DefaultBlockCount);
        auto cps = config.MutableExplicitChannelProfiles();
        cps->Add()->SetDataKind(static_cast<ui32>(EChannelDataKind::System));
        cps->Add()->SetDataKind(static_cast<ui32>(EChannelDataKind::Log));
        cps->Add()->SetDataKind(static_cast<ui32>(EChannelDataKind::Index));
        cps->Add()->SetDataKind(static_cast<ui32>(EChannelDataKind::Merged));
        cps->Add()->SetDataKind(static_cast<ui32>(EChannelDataKind::Merged));
        cps->Add()->SetDataKind(static_cast<ui32>(EChannelDataKind::Mixed));
        cps->Add()->SetDataKind(static_cast<ui32>(EChannelDataKind::Merged));
        cps->Add()->SetDataKind(static_cast<ui32>(EChannelDataKind::Merged));
        cps->Add()->SetDataKind(static_cast<ui32>(EChannelDataKind::Merged));
        cps->Add()->SetDataKind(static_cast<ui32>(EChannelDataKind::Merged));
        cps->Add()->SetDataKind(static_cast<ui32>(EChannelDataKind::Mixed));

        TPartitionState state(
            meta,
            0,  // generation
            BuildDefaultCompactionPolicy(5),
            0,  // compactionScoreHistorySize
            0,  // cleanupScoreHistorySize
            DefaultBPConfig(),
            DefaultFreeSpaceConfig(),
            Max(),  // maxIORequestsInFlight
            0,  // lastCommitId
            config.ExplicitChannelProfilesSize(),
            0,  // mixedIndexCacheSize
            false  // calculateLogicalUsedBlocks
        );

        const auto perm = EChannelPermission::UserWritesAllowed;
        auto kind = EChannelDataKind::Merged;

        auto blobId = state.GenerateBlobId(kind, perm, 1, 1024);
        UNIT_ASSERT(blobId.Channel() == 3);

        kind = EChannelDataKind::Mixed;

        blobId = state.GenerateBlobId(kind, perm, 1, 1024);
        UNIT_ASSERT(blobId.Channel() == 5);
        blobId = state.GenerateBlobId(kind, perm, 1, 1024);
        UNIT_ASSERT(blobId.Channel() == 10);
        blobId = state.GenerateBlobId(kind, perm, 1, 1024);
        UNIT_ASSERT(blobId.Channel() == 5);

        kind = EChannelDataKind::Merged;

        blobId = state.GenerateBlobId(kind, perm, 1, 1024);
        UNIT_ASSERT(blobId.Channel() == 4);
        blobId = state.GenerateBlobId(kind, perm, 1, 1024);
        UNIT_ASSERT(blobId.Channel() == 6);
        blobId = state.GenerateBlobId(kind, perm, 1, 1024);
        UNIT_ASSERT(blobId.Channel() == 7);
        blobId = state.GenerateBlobId(kind, perm, 1, 1024);
        UNIT_ASSERT(blobId.Channel() == 8);
        blobId = state.GenerateBlobId(kind, perm, 1, 1024);
        UNIT_ASSERT(blobId.Channel() == 9);
        blobId = state.GenerateBlobId(kind, perm, 1, 1024);
        UNIT_ASSERT(blobId.Channel() == 3);
    }

    Y_UNIT_TEST(PickNextChannelWithProperFreeSpaceShare)
    {
        for (auto kind: {EChannelDataKind::Mixed, EChannelDataKind::Merged}) {
            TPartitionState state(
                DefaultConfig(2, DefaultBlockCount),
                0,  // generation
                BuildDefaultCompactionPolicy(5),
                0,  // compactionScoreHistorySize
                0,  // cleanupScoreHistorySize
                DefaultBPConfig(),
                DefaultFreeSpaceConfig(),
                Max(),  // maxIORequestsInFlight
                0,  // lastCommitId
                6,  // channelCount
                0,  // mixedIndexCacheSize
                false  // calculateLogicalUsedBlocks
            );

            {
                auto blobId = state.GenerateBlobId(
                    kind,
                    EChannelPermission::UserWritesAllowed,
                    1,
                    1024
                );
                UNIT_ASSERT_VALUES_EQUAL(
                    ui32(TPartitionSchema::FirstDataChannel),
                    blobId.Channel()
                );
                auto blobId2 = state.GenerateBlobId(
                    kind,
                    EChannelPermission::UserWritesAllowed,
                    1,
                    1024
                );
                UNIT_ASSERT_VALUES_EQUAL(
                    ui32(TPartitionSchema::FirstDataChannel + 1),
                    blobId2.Channel()
                );
            }

            UNIT_ASSERT_VALUES_EQUAL(0, state.GetAlmostFullChannelCount());
            state.UpdateChannelFreeSpaceShare(
                TPartitionSchema::FirstDataChannel,
                0.15
            );
            UNIT_ASSERT_VALUES_EQUAL(1, state.GetAlmostFullChannelCount());

            for (ui32 i = 0; i < 10; ++i) {
                auto blobId = state.GenerateBlobId(
                    kind,
                    EChannelPermission::UserWritesAllowed,
                    1,
                    1024
                );
                UNIT_ASSERT_VALUES_EQUAL(
                    ui32(TPartitionSchema::FirstDataChannel + 1),
                    blobId.Channel()
                );
            }

            state.UpdateChannelFreeSpaceShare(
                TPartitionSchema::FirstDataChannel,
                0.16
            );
            UNIT_ASSERT_VALUES_EQUAL(1, state.GetAlmostFullChannelCount());

            ui32 firstChannelSelected = 0;
            ui32 secondChannelSelected = 0;
            for (ui32 i = 0; i < 1000; ++i) {
                auto blobId = state.GenerateBlobId(
                    kind,
                    EChannelPermission::UserWritesAllowed,
                    1,
                    1024
                );
                if (blobId.Channel() == TPartitionSchema::FirstDataChannel) {
                    ++firstChannelSelected;
                } else {
                    UNIT_ASSERT_VALUES_EQUAL(
                        ui32(TPartitionSchema::FirstDataChannel + 1),
                        blobId.Channel()
                    );
                    ++secondChannelSelected;
                }
            }

            UNIT_ASSERT(firstChannelSelected < 150 && firstChannelSelected > 50);

            state.UpdateChannelFreeSpaceShare(
                TPartitionSchema::FirstDataChannel,
                0.16
            );
            state.UpdateChannelFreeSpaceShare(
                TPartitionSchema::FirstDataChannel + 1,
                0.161
            );
            UNIT_ASSERT_VALUES_EQUAL(2, state.GetAlmostFullChannelCount());

            firstChannelSelected = 0;
            secondChannelSelected = 0;
            for (ui32 i = 0; i < 1000; ++i) {
                auto blobId = state.GenerateBlobId(
                    kind,
                    EChannelPermission::UserWritesAllowed,
                    1,
                    1024
                );
                if (blobId.Channel() == TPartitionSchema::FirstDataChannel) {
                    ++firstChannelSelected;
                } else {
                    UNIT_ASSERT_VALUES_EQUAL(
                        ui32(TPartitionSchema::FirstDataChannel + 1),
                        blobId.Channel()
                    );
                    ++secondChannelSelected;
                }
            }

            UNIT_ASSERT(firstChannelSelected < 150 && firstChannelSelected > 50);
        }
    }

    Y_UNIT_TEST(PickMergedChannelIfAllMixedChannelsAreFull)
    {
        NProto::TPartitionMeta meta;
        auto& config = *meta.MutableConfig();
        config.SetBlockSize(DefaultBlockSize);
        config.SetBlocksCount(DefaultBlockCount);
        config.AddExplicitChannelProfiles()->SetDataKind(
            static_cast<ui32>(EChannelDataKind::System));
        config.AddExplicitChannelProfiles()->SetDataKind(
            static_cast<ui32>(EChannelDataKind::Log));
        config.AddExplicitChannelProfiles()->SetDataKind(
            static_cast<ui32>(EChannelDataKind::Index));
        config.AddExplicitChannelProfiles()->SetDataKind(
            static_cast<ui32>(EChannelDataKind::Mixed));
        config.AddExplicitChannelProfiles()->SetDataKind(
            static_cast<ui32>(EChannelDataKind::Merged));

        TPartitionState state(
            meta,
            0,  // generation
            BuildDefaultCompactionPolicy(5),
            0,  // compactionScoreHistorySize
            0,  // cleanupScoreHistorySize
            DefaultBPConfig(),
            DefaultFreeSpaceConfig(),
            Max(),  // maxIORequestsInFlight
            0,  // lastCommitId
            config.ExplicitChannelProfilesSize(),
            0,  // mixedIndexCacheSize
            false  // calculateLogicalUsedBlocks
        );

        {
            auto mixedBlobId = state.GenerateBlobId(
                EChannelDataKind::Mixed,
                EChannelPermission::UserWritesAllowed,
                1,
                1024
            );
            UNIT_ASSERT_VALUES_EQUAL(
                ui32(TPartitionSchema::FirstDataChannel),
                mixedBlobId.Channel()
            );
            auto mergedBlobId = state.GenerateBlobId(
                EChannelDataKind::Merged,
                EChannelPermission::UserWritesAllowed,
                1,
                1024
            );
            UNIT_ASSERT_VALUES_EQUAL(
                ui32(TPartitionSchema::FirstDataChannel + 1),
                mergedBlobId.Channel()
            );
        }

        state.UpdatePermissions(TPartitionSchema::FirstDataChannel, {});

        {
            auto mixedBlobId = state.GenerateBlobId(
                EChannelDataKind::Mixed,
                EChannelPermission::UserWritesAllowed,
                1,
                1024
            );
            UNIT_ASSERT_VALUES_EQUAL(
                ui32(TPartitionSchema::FirstDataChannel + 1),
                mixedBlobId.Channel()
            );
        }
    }

    Y_UNIT_TEST(CalculateCurrentBackpressure)
    {
        TPartitionState state(
            DefaultConfig(1, 1000),
            0,  // generation
            BuildDefaultCompactionPolicy(5),
            0,  // compactionScoreHistorySize
            0,  // cleanupScoreHistorySize
            DefaultBPConfig(),
            DefaultFreeSpaceConfig(),
            Max(),  // maxIORequestsInFlight
            0,  // lastCommitId
            5,  // channelCount
            0,  // mixedIndexCacheSize
            false  // calculateLogicalUsedBlocks
        );

        const auto initialBackpressure = state.CalculateCurrentBackpressure();
        UNIT_ASSERT_VALUES_EQUAL(1, initialBackpressure.FreshIndexScore);
        UNIT_ASSERT_VALUES_EQUAL(1, initialBackpressure.CompactionScore);
        UNIT_ASSERT_VALUES_EQUAL(1, initialBackpressure.DiskSpaceScore);

        state.IncrementUnflushedFreshBlocksFromDbCount(100);
        state.GetCompactionMap().Update(0, 10, 10, 10, false);

        const auto marginalBackpressure = state.CalculateCurrentBackpressure();
        UNIT_ASSERT_DOUBLES_EQUAL(1, marginalBackpressure.FreshIndexScore, 1e-5);
        UNIT_ASSERT_DOUBLES_EQUAL(1, marginalBackpressure.CompactionScore, 1e-5);

        // Backpressure caused by increased FreshBlobByteCount
        {
            state.AddFreshBlob({ 1, 50 * 4096 });

            const auto bp = state.CalculateCurrentBackpressure();
            UNIT_ASSERT_DOUBLES_EQUAL(2.5, bp.FreshIndexScore, 1e-5);
        }

        state.IncrementUnflushedFreshBlocksFromDbCount(300);
        state.GetCompactionMap().Update(0, 30, 30, 30, false);

        const auto maxBackpressure = state.CalculateCurrentBackpressure();
        UNIT_ASSERT_DOUBLES_EQUAL(10, maxBackpressure.FreshIndexScore, 1e-5);
        UNIT_ASSERT_DOUBLES_EQUAL(10, maxBackpressure.CompactionScore, 1e-5);

        state.GetCompactionMap().Update(0, 100, 100, 100, false);

        const auto maxBackpressure2 = state.CalculateCurrentBackpressure();
        UNIT_ASSERT_DOUBLES_EQUAL(10, maxBackpressure2.CompactionScore, 1e-5);
    }

    Y_UNIT_TEST(CompactionBackpressureShouldBeZeroIfNotRequiredByPolicy)
    {
        TPartitionState state(
            DefaultConfig(1, 1000),
            0,  // generation
            std::make_shared<TNoBackpressurePolicy>(),
            0,  // compactionScoreHistorySize
            0,  // cleanupScoreHistorySize
            DefaultBPConfig(),
            DefaultFreeSpaceConfig(),
            Max(),  // maxIORequestsInFlight
            0,  // lastCommitId
            5,  // channelCount
            0,  // mixedIndexCacheSize
            false  // calculateLogicalUsedBlocks
        );

        state.GetCompactionMap().Update(0, 30, 30, 30, false);

        const auto bp = state.CalculateCurrentBackpressure();
        UNIT_ASSERT_VALUES_EQUAL(0, bp.CompactionScore);
    }

    Y_UNIT_TEST(ShouldReturnInvalidCommitIdWhenItOverflows)
    {
        TPartitionState state(
            DefaultConfig(1, DefaultBlockCount),
            0,  // generation
            BuildDefaultCompactionPolicy(5),
            0,  // compactionScoreHistorySize
            0,  // cleanupScoreHistorySize
            DefaultBPConfig(),
            DefaultFreeSpaceConfig(),
            Max(),  // maxIORequestsInFlight
            Max(),  // lastCommitId
            5,  // channelCount
            0,  // mixedIndexCacheSize
            false  // calculateLogicalUsedBlocks
        );

        UNIT_ASSERT(state.GenerateCommitId() == InvalidCommitId);
    }

    Y_UNIT_TEST(ShouldCorrectlyCalculateUsedBlocksCount)
    {
        auto config = DefaultConfig(1, DefaultBlockCount);

        config.MutableConfig()->SetBaseDiskId("baseDiskID");
        config.MutableConfig()->SetBaseDiskCheckpointId("baseDiskCheckpointId");

        TPartitionState state(
            config,
            0,  // generation
            BuildDefaultCompactionPolicy(5),
            0,  // compactionScoreHistorySize
            0,  // cleanupScoreHistorySize
            DefaultBPConfig(),
            DefaultFreeSpaceConfig(),
            Max(),  // maxIORequestsInFlight
            0,  // lastCommitId
            5,  // channelCount
            0,  // mixedIndexCacheSize
            true  // calculateLogicalUsedBlocks
        );

        state.GetLogicalUsedBlocks().Set(0, 9);
        state.IncrementLogicalUsedBlocksCount(10);

        TTestExecutor executor;
        executor.WriteTx([&] (TPartitionDatabase db) {
            db.InitSchema();
        });

        executor.WriteTx([&] (TPartitionDatabase db) {
            state.SetUsedBlocks(db, TBlockRange32(100, 110), 0);
        });
        UNIT_ASSERT_EQUAL(11, state.GetUsedBlocksCount());
        UNIT_ASSERT_EQUAL(21, state.GetLogicalUsedBlocksCount());

        executor.WriteTx([&] (TPartitionDatabase db) {
            state.SetUsedBlocks(db, TBlockRange32(105, 130), 0);
        });
        UNIT_ASSERT_EQUAL(31, state.GetUsedBlocksCount());
        UNIT_ASSERT_EQUAL(41, state.GetLogicalUsedBlocksCount());

        executor.WriteTx([&] (TPartitionDatabase db) {
            state.UnsetUsedBlocks(db, TBlockRange32(106, 115));
        });
        UNIT_ASSERT_EQUAL(21, state.GetUsedBlocksCount());
        UNIT_ASSERT_EQUAL(31, state.GetLogicalUsedBlocksCount());

        executor.WriteTx([&] (TPartitionDatabase db) {
            state.UnsetUsedBlocks(db, TBlockRange32(109, 110));
        });
        UNIT_ASSERT_EQUAL(21, state.GetUsedBlocksCount());
        UNIT_ASSERT_EQUAL(31, state.GetLogicalUsedBlocksCount());

        executor.WriteTx([&] (TPartitionDatabase db) {
            state.SetUsedBlocks(db, {101, 102, 103, 106, 108});
        });
        UNIT_ASSERT_EQUAL(23, state.GetUsedBlocksCount());
        UNIT_ASSERT_EQUAL(33, state.GetLogicalUsedBlocksCount());

        executor.WriteTx([&] (TPartitionDatabase db) {
            state.UnsetUsedBlocks(db, {108, 120, 250});
        });
        UNIT_ASSERT_EQUAL(21, state.GetUsedBlocksCount());
        UNIT_ASSERT_EQUAL(31, state.GetLogicalUsedBlocksCount());
    }

    Y_UNIT_TEST(ShouldCalculateFreshBlobByteCount)
    {
        TPartitionState state(
            DefaultConfig(1, DefaultBlockCount),
            0,  // generation
            BuildDefaultCompactionPolicy(5),
            0,  // compactionScoreHistorySize
            0,  // cleanupScoreHistorySize
            DefaultBPConfig(),
            DefaultFreeSpaceConfig(),
            Max(),  // maxIORequestsInFlight
            0,  // lastCommitId
            5,  // channelCount
            0,  // mixedIndexCacheSize
            false  // calculateLogicalUsedBlocks
        );

        state.AddFreshBlob({1, 10});
        state.AddFreshBlob({3, 30});
        state.AddFreshBlob({2, 20});
        state.AddFreshBlob({5, 50});
        state.AddFreshBlob({4, 40});

        UNIT_ASSERT_VALUES_EQUAL(150, state.GetUntrimmedFreshBlobByteCount());

        state.TrimFreshBlobs(3);

        UNIT_ASSERT_VALUES_EQUAL(90, state.GetUntrimmedFreshBlobByteCount());

        state.AddFreshBlob({7, 70});

        UNIT_ASSERT_VALUES_EQUAL(160, state.GetUntrimmedFreshBlobByteCount());

        state.TrimFreshBlobs(10);

        UNIT_ASSERT_VALUES_EQUAL(0, state.GetUntrimmedFreshBlobByteCount());
    }

    Y_UNIT_TEST(ShouldCorrectlyCalculateCheckpointBytes)
    {
        auto config = DefaultConfig(1, 10_GB / DefaultBlockSize);

        TPartitionState state(
            config,
            0,  // generation
            BuildDefaultCompactionPolicy(5),
            0,  // compactionScoreHistorySize
            0,  // cleanupScoreHistorySize
            DefaultBPConfig(),
            DefaultFreeSpaceConfig(),
            Max(),  // maxIORequestsInFlight
            0,  // lastCommitId
            1,  // channelCount
            0,  // mixedIndexCacheSize
            true  // calculateLogicalUsedBlocks
        );

        state.IncrementMergedBlocksCount(5_GB / DefaultBlockSize);
        TCheckpoint checkpoint;
        checkpoint.CheckpointId = "c1";
        checkpoint.CommitId = 1;
        checkpoint.Stats.CopyFrom(state.GetStats());
        state.GetCheckpoints().Add(checkpoint);

        state.IncrementMixedBlocksCount(2_GB / DefaultBlockSize);

        checkpoint.CheckpointId = "c2";
        checkpoint.CommitId = 2;
        checkpoint.Stats.CopyFrom(state.GetStats());
        state.GetCheckpoints().Add(checkpoint);

        UNIT_ASSERT_VALUES_EQUAL(7_GB, state.CalculateCheckpointBytes());
    }

    Y_UNIT_TEST(ShouldStoreBlocksInMixedCache)
    {
        auto config = DefaultConfig(1, 10_GB / DefaultBlockSize);

        TPartitionState state(
            config,
            0,  // generation
            BuildDefaultCompactionPolicy(5),
            0,  // compactionScoreHistorySize
            0,  // cleanupScoreHistorySize
            DefaultBPConfig(),
            DefaultFreeSpaceConfig(),
            Max(),  // maxIORequestsInFlight
            0,  // lastCommitId
            5,  // channelCount
            1,  // mixedIndexCacheSize
            false  // calculateLogicalUsedBlocks
        );

        TTestExecutor executor;
        executor.WriteTx([&] (TPartitionDatabase db) {
            db.InitSchema();
        });

        constexpr ui32 rangeIdx = 0;
        TVector<TMixedBlock> blocks = {
            { {1, 1}, 1, 1, 1 },
            { {2, 2}, 2, 2, 2 },
            { {3, 3}, 3, 3, 3 },
            { {4, 4}, 4, 4, 4 },
            { {5, 5}, 5, 5, 5 }
        };

        auto mixedBlocksCompatator = [](const auto& lhs, const auto& rhs) {
            return lhs.BlockIndex < rhs.BlockIndex;
        };

        // range is warm now: mixed blocks are not cached
        state.RaiseRangeTemperature(rangeIdx);

        executor.WriteTx([&] (TPartitionDatabase db) {
            state.WriteMixedBlock(db, blocks[0]);
            state.WriteMixedBlock(db, blocks[1]);
        });

        TVector<TMixedBlock> actual;

        struct TVisitor final
            : public IBlocksIndexVisitor
        {
            TVector<TMixedBlock>& Blocks;

            TVisitor(TVector<TMixedBlock>& blocks)
                : Blocks(blocks)
            {}

            bool Visit(
                ui32 blockIndex,
                ui64 commitId,
                const TPartialBlobId& blobId,
                ui16 blobOffset) override
            {
                Blocks.emplace_back(blobId, commitId, blockIndex, blobOffset);
                return true;
            }

        } visitor{actual};

        // should read mixed blocks from db and place them into cache
        executor.WriteTx([&] (TPartitionDatabase db) {
            state.FindMixedBlocksForCompaction(db, visitor, rangeIdx);
        });

        Sort(actual, mixedBlocksCompatator);
        ASSERT_VECTORS_EQUAL(
            TVector<TMixedBlock>({blocks[0], blocks[1]}),
            actual
        );

        // range is hot now
        state.RaiseRangeTemperature(rangeIdx);

        executor.WriteTx([&] (TPartitionDatabase db) {
            state.DeleteMixedBlock(db, blocks[1].BlockIndex, blocks[1].CommitId);
            state.WriteMixedBlock(db, blocks[2]);
            state.WriteMixedBlock(db, blocks[3]);
        });

        actual.clear();

        executor.WriteTx([&] (TPartitionDatabase db) {
            state.FindMixedBlocksForCompaction(db, visitor, rangeIdx);
        });

        Sort(actual, mixedBlocksCompatator);
        ASSERT_VECTORS_EQUAL(
            TVector<TMixedBlock>({blocks[0], blocks[2], blocks[3]}),
            actual
        );

        // kick range from cache
        state.RaiseRangeTemperature(rangeIdx + 1);

        executor.WriteTx([&] (TPartitionDatabase db) {
            state.DeleteMixedBlock(db, blocks[2].BlockIndex, blocks[2].CommitId);
            state.WriteMixedBlock(db, blocks[4]);
        });

        actual.clear();

        // should read from db
        executor.WriteTx([&] (TPartitionDatabase db) {
            state.FindMixedBlocksForCompaction(db, visitor, rangeIdx);
        });

        Sort(actual, mixedBlocksCompatator);
        ASSERT_VECTORS_EQUAL(
            TVector<TMixedBlock>({blocks[0], blocks[3], blocks[4]}),
            actual
        );
    }
}

}   // namespace NCloud::NBlockStore::NStorage::NPartition

template <>
inline void Out<NCloud::NBlockStore::NStorage::NPartition::TMixedBlock>(
    IOutputStream& out,
    const NCloud::NBlockStore::NStorage::NPartition::TMixedBlock& b)
{
    out << "[" << b.BlockIndex << ", " << b.CommitId << ", "
        << b.BlobId << ", " << b.BlobOffset << "]";
}
