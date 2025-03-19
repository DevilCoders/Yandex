#include <cloud/blockstore/libs/storage/testlib/test_env.h>

#include <cloud/blockstore/libs/storage/core/features_config.h>
#include <cloud/blockstore/libs/storage/volume_balancer/volume_balancer_state.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/datetime/base.h>
#include <util/generic/vector.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

namespace {

////////////////////////////////////////////////////////////////////////////////

using TVolumeUsage = std::pair<ui64, ui64>;

constexpr TVolumeUsage VolumeUsageNo {0, 0};
constexpr TVolumeUsage VolumeUsageHigh {1e6, 1e6};
constexpr TVolumeUsage VolumeUsageLow {1e5, 1e5};

////////////////////////////////////////////////////////////////////////////////

TStorageConfigPtr CreateStorageConfig(
    ui32 pushPercents,
    ui32 pullPercents,
    ui64 matBenchNsSys,
    ui64 matBenchNsUser,
    NProto::EVolumePreemptionType type,
    TFeaturesConfigPtr featuresConfig)
{
    NProto::TStorageServiceConfig storageConfig;
    storageConfig.SetVolumePreemptionType(type);
    storageConfig.SetPreemptionPushPercentage(pushPercents);
    storageConfig.SetPreemptionPushPercentage(pullPercents);
    storageConfig.SetCpuMatBenchNsSystemThreshold(matBenchNsSys);
    storageConfig.SetCpuMatBenchNsUserThreshold(matBenchNsUser);
    if (!featuresConfig) {
        NProto::TFeaturesConfig config;
        featuresConfig = std::make_shared<TFeaturesConfig>(config);
    }

    return std::make_shared<TStorageConfig>(storageConfig, std::move(featuresConfig));
};

TFeaturesConfigPtr CreateFeatureConfig(
    const TString& featureName,
    const TVector<TString>& list,
    bool blacklist)
{
    NProto::TFeaturesConfig config;
    if (featureName) {
        auto* feature = config.MutableFeatures()->Add();
        feature->SetName(featureName);
        if (blacklist) {
            for (const auto& c: list) {
                *feature->MutableBlacklist()->MutableCloudIds()->Add() = c;
            }
        } else {
            for (const auto& c: list) {
                *feature->MutableWhitelist()->MutableCloudIds()->Add() = c;
            }
        }
    }
    return std::make_shared<TFeaturesConfig>(config);
}

NProto::TVolumeBalancerDiskStats CreateVolumeStats(
    const TString& diskId,
    const TString& cloudId,
    TVolumeUsage usage,
    bool isLocal,
    NProto::EPreemptionSource source)
{
    NProto::TVolumeBalancerDiskStats stats;
    stats.SetDiskId(diskId);
    stats.SetCloudId(cloudId);
    stats.SetSystemCpu(usage.first);
    stats.SetUserCpu(usage.second);
    stats.SetIsLocal(isLocal);
    stats.SetPreemptionSource(source);

    return stats;
}

NProto::TVolumeBalancerDiskStats CreateVolumeStats(
    const TString& diskId,
    const TString& cloudId,
    TVolumeUsage usage,
    bool isLocal)
{
    NProto::TVolumeBalancerDiskStats stats;
    stats.SetDiskId(diskId);
    stats.SetCloudId(cloudId);
    stats.SetSystemCpu(usage.first);
    stats.SetUserCpu(usage.second);
    stats.SetIsLocal(isLocal);
    stats.SetPreemptionSource(NProto::EPreemptionSource::SOURCE_BALANCER);

    return stats;
}

void RunState(
    TVolumeBalancerState& state,
    const TVector<NProto::TVolumeBalancerDiskStats>& vols,
    TInstant now,
    ui32 cpuLoad)
{
    TVolumeBalancerState::TPerfGuaranteesMap perfMap;
    for (ui32 i = 0; i < 5; ++i) {
        state.UpdateVolumeStats(vols, std::move(perfMap), cpuLoad, cpuLoad, 0, 0, 0, now);
        now += TDuration::Seconds(15);
    }
}

void RunState(
    TVolumeBalancerState& state,
    const TVector<NProto::TVolumeBalancerDiskStats>& vols,
    TVolumeBalancerState::TPerfGuaranteesMap perfMap,
    TInstant now,
    ui32 cpuLoad)
{
    for (ui32 i = 0; i < 5; ++i) {
        state.UpdateVolumeStats(vols, std::move(perfMap), cpuLoad, cpuLoad, 0, 0, 0, now);
        now += TDuration::Seconds(15);
    }
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TVolumeBalancerStateTest)
{
    Y_UNIT_TEST(ShouldSelectHaviestVolumeForPreemption)
    {
        TVolumeBalancerState state(
            CreateStorageConfig(
                80,
                40,
                100,
                100,
                NProto::PREEMPTION_MOVE_MOST_HEAVY,
                CreateFeatureConfig("Balancer", {}, true)
            )
        );
        TInstant now = TInstant::Seconds(0);

        TVector<NProto::TVolumeBalancerDiskStats> vols {
            CreateVolumeStats("vol0", "", VolumeUsageHigh, true),
            CreateVolumeStats("vol1", "", VolumeUsageLow, true)};

        RunState(state, vols, now, 90);

        UNIT_ASSERT_VALUES_EQUAL("vol0", state.GetVolumeToPush());
        UNIT_ASSERT(!state.GetVolumeToPull());
        UNIT_ASSERT_VALUES_EQUAL(
            static_cast<ui32>(EBalancerState::STATE_HI_LOAD),
            state.GetState());
    }

    Y_UNIT_TEST(ShouldSelectLightestVolumeForPreemption)
    {
        TVolumeBalancerState state(
            CreateStorageConfig(
                80,
                40,
                100,
                100,
                NProto::PREEMPTION_MOVE_LEAST_HEAVY,
                CreateFeatureConfig("Balancer", {}, true)
            )
        );
        TInstant now = TInstant::Seconds(0);

        TVector<NProto::TVolumeBalancerDiskStats> vols {
            CreateVolumeStats("vol0", "", VolumeUsageHigh, true),
            CreateVolumeStats("vol1", "", VolumeUsageLow, true) };

        RunState(state, vols, now, 90);

        UNIT_ASSERT_VALUES_EQUAL("vol1", state.GetVolumeToPush());
        UNIT_ASSERT(!state.GetVolumeToPull());
        UNIT_ASSERT_VALUES_EQUAL(
            static_cast<ui32>(EBalancerState::STATE_HI_LOAD),
            state.GetState());
    }

    Y_UNIT_TEST(ShouldNotPreemptVolumeIfLoadIsBelowThreshold)
    {
        TVolumeBalancerState state(
            CreateStorageConfig(
                80,
                80,
                100,
                100,
                NProto::PREEMPTION_MOVE_MOST_HEAVY,
                CreateFeatureConfig("Balancer", {}, true)
            )
        );
        TInstant now = TInstant::Seconds(0);

        TVector<NProto::TVolumeBalancerDiskStats> vols {
            CreateVolumeStats("vol0", "", VolumeUsageHigh, false),
            CreateVolumeStats("vol1", "", VolumeUsageLow, false) };

        RunState(state, vols, now, 50);

        UNIT_ASSERT(!state.GetVolumeToPush());
        UNIT_ASSERT(!state.GetVolumeToPull());
        UNIT_ASSERT_VALUES_EQUAL(
            static_cast<ui32>(EBalancerState::STATE_MEASURING),
            state.GetState());
    }

    Y_UNIT_TEST(ShouldReturnVolumeIfLoadIsBelowThresholdAnsDiskSuffersAtSvm)
    {
        auto storageConfig = CreateStorageConfig(
            80,
            40,
            100,
            100,
            NProto::PREEMPTION_MOVE_MOST_HEAVY,
            CreateFeatureConfig("Balancer", {}, true));

        TVolumeBalancerState state(storageConfig);
        TInstant now = TInstant::Seconds(0);

        {
            TVector<NProto::TVolumeBalancerDiskStats> vols {
                CreateVolumeStats("vol0", "", VolumeUsageHigh, true),
                CreateVolumeStats("vol1", "", VolumeUsageLow, true) };

            RunState(state, vols, now, 90);

            UNIT_ASSERT_VALUES_EQUAL("vol0", state.GetVolumeToPush());
            UNIT_ASSERT(!state.GetVolumeToPull());
            UNIT_ASSERT_VALUES_EQUAL(
                static_cast<ui32>(EBalancerState::STATE_HI_LOAD),
                state.GetState());
        }

        {
            TVector<NProto::TVolumeBalancerDiskStats> vols {
                CreateVolumeStats("vol0", "", VolumeUsageNo, false),
                CreateVolumeStats("vol1", "", VolumeUsageLow, false) };

            TVolumeBalancerState::TPerfGuaranteesMap perfMap;
            perfMap["vol0"] = true;
            perfMap["vol1"] = true;

            RunState(state, vols, perfMap, now, 30);

            UNIT_ASSERT(!state.GetVolumeToPush());
            UNIT_ASSERT(!state.GetVolumeToPull());
            UNIT_ASSERT_VALUES_EQUAL(
                static_cast<ui32>(EBalancerState::STATE_LOW_LOAD),
                state.GetState());

            now += storageConfig->GetInitialPullDelay();

            state.UpdateVolumeStats(vols, perfMap, 30, 30, 0, 0, 0, now);
            UNIT_ASSERT(!state.GetVolumeToPush());
            UNIT_ASSERT(state.GetVolumeToPull());
        }
    }

    Y_UNIT_TEST(ShouldNotSelectVolumeFromBlacklistedCloud)
    {
        TVolumeBalancerState state(
            CreateStorageConfig(
                80,
                40,
                100,
                100,
                NProto::PREEMPTION_MOVE_MOST_HEAVY,
                CreateFeatureConfig("Balancer", {"cloudid1"}, true)
            )
        );
        TInstant now = TInstant::Seconds(0);

        TVector<NProto::TVolumeBalancerDiskStats> vols {
            CreateVolumeStats("vol0", "cloudid1", VolumeUsageHigh, true),
            CreateVolumeStats("vol1", "cloudid2", VolumeUsageLow, true) };

        RunState(state, vols, now, 90);

        UNIT_ASSERT_VALUES_EQUAL("vol1", state.GetVolumeToPush());
        UNIT_ASSERT(!state.GetVolumeToPull());
        UNIT_ASSERT_VALUES_EQUAL(
            static_cast<ui32>(EBalancerState::STATE_HI_LOAD),
            state.GetState());
    }

    Y_UNIT_TEST(ShouldReturnVolumeOnlyAfterPullDelay)
    {
        auto storageConfig = CreateStorageConfig(
            80,
            40,
            100,
            100,
            NProto::PREEMPTION_MOVE_MOST_HEAVY,
            CreateFeatureConfig("Balancer", {}, true));

        TVolumeBalancerState state(storageConfig);
        TInstant now = TInstant::Seconds(0);

        {
            TVector<NProto::TVolumeBalancerDiskStats> vols {
                CreateVolumeStats("vol0", "", VolumeUsageHigh, true),
                CreateVolumeStats("vol1", "", VolumeUsageLow, true) };

            RunState(state, vols, now, 90);

            UNIT_ASSERT(state.GetVolumeToPush() == "vol0");
            UNIT_ASSERT(!state.GetVolumeToPull());
            UNIT_ASSERT_VALUES_EQUAL(
                static_cast<ui32>(EBalancerState::STATE_HI_LOAD),
                state.GetState());
        }

        {
            TVector<NProto::TVolumeBalancerDiskStats> vols {
                CreateVolumeStats("vol0", "", VolumeUsageNo, false),
                CreateVolumeStats("vol1", "", VolumeUsageLow, true) };

            TVolumeBalancerState::TPerfGuaranteesMap perfMap;
            perfMap["vol0"] = true;
            perfMap["vol1"] = true;

            RunState(state, vols, perfMap,  now, 30);

            UNIT_ASSERT(!state.GetVolumeToPush());

            now += storageConfig->GetInitialPullDelay();

            state.UpdateVolumeStats(vols, perfMap, 30, 30, 0, 0, 0, now);

            UNIT_ASSERT(!state.GetVolumeToPush());
            UNIT_ASSERT(state.GetVolumeToPull() == "vol0");
        }
    }

    Y_UNIT_TEST(ShouldNotReturnManuallyPreemptedVolume)
    {
        TVolumeBalancerState state(
            CreateStorageConfig(
                80,
                40,
                100,
                100,
                NProto::PREEMPTION_MOVE_MOST_HEAVY,
                CreateFeatureConfig("Balancer", {}, true)
            )
        );
        TInstant now = TInstant::Seconds(0);

        {
            TVector<NProto::TVolumeBalancerDiskStats> vols {
                CreateVolumeStats("vol0", "", VolumeUsageHigh, true),
                CreateVolumeStats("vol1", "", VolumeUsageLow, true) };

            RunState(state, vols, now, 90);

            UNIT_ASSERT(state.GetVolumeToPush() == "vol0");
            UNIT_ASSERT(!state.GetVolumeToPull());
            UNIT_ASSERT_VALUES_EQUAL(
                static_cast<ui32>(EBalancerState::STATE_HI_LOAD),
                state.GetState());
        }

        {
            TVector<NProto::TVolumeBalancerDiskStats> vols {
                CreateVolumeStats("vol0", "", VolumeUsageHigh, false, NProto::EPreemptionSource::SOURCE_INITIAL_MOUNT),
                CreateVolumeStats("vol1", "", VolumeUsageLow, true, NProto::EPreemptionSource::SOURCE_INITIAL_MOUNT) };

            RunState(state, vols, now, 30);

            UNIT_ASSERT(!state.GetVolumeToPush());
            UNIT_ASSERT(!state.GetVolumeToPull());
            UNIT_ASSERT_VALUES_EQUAL(
                static_cast<ui32>(EBalancerState::STATE_LOW_LOAD),
                state.GetState());
        }
    }

    Y_UNIT_TEST(ShouldPreemptVolumeIfCpuMatBenchNsIsAboveThresold)
    {
        TVolumeBalancerState state(
            CreateStorageConfig(
                80,
                40,
                100,
                100,
                NProto::PREEMPTION_MOVE_MOST_HEAVY,
                CreateFeatureConfig("Balancer", {}, true)
            )
        );
        TInstant now = TInstant::Seconds(0);

        TVector<NProto::TVolumeBalancerDiskStats> vols {
            CreateVolumeStats("vol0", "", VolumeUsageHigh, true),
            CreateVolumeStats("vol1", "", VolumeUsageLow, true) };

        TVolumeBalancerState::TPerfGuaranteesMap perfMap;

        state.UpdateVolumeStats(vols, std::move(perfMap), 0, 0, 101, 101, 0, now);

        UNIT_ASSERT_VALUES_EQUAL("vol0", state.GetVolumeToPush());
        UNIT_ASSERT(!state.GetVolumeToPull());
    }

    Y_UNIT_TEST(ShouldPreemptVolumeIfCpuWaitIsAboveThresold)
    {
        TVolumeBalancerState state(
            CreateStorageConfig(
                80,
                40,
                100,
                100,
                NProto::PREEMPTION_MOVE_MOST_HEAVY,
                CreateFeatureConfig("Balancer", {}, true)
            )
        );
        TInstant now = TInstant::Seconds(0);

        UNIT_ASSERT(!state.GetVolumeToPush());

        TVector<NProto::TVolumeBalancerDiskStats> vols;
        vols.push_back(CreateVolumeStats("vol0", "", VolumeUsageHigh, true));
        vols.push_back(CreateVolumeStats("vol1", "", VolumeUsageLow, true));

        TVolumeBalancerState::TPerfGuaranteesMap perfMap;

        state.UpdateVolumeStats(vols, std::move(perfMap), 0, 0, 0, 0, 110, now);

        UNIT_ASSERT_VALUES_EQUAL("vol0", state.GetVolumeToPush());
        UNIT_ASSERT(!state.GetVolumeToPull());
    }
}

}   // namespace NCloud::NBlockStore::NStorage
