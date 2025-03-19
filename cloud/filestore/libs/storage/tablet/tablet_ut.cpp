#include "tablet.h"

#include <cloud/filestore/libs/diagnostics/critical_events.h>
#include <cloud/filestore/libs/storage/testlib/tablet_client.h>
#include <cloud/filestore/libs/storage/testlib/test_env.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NCloud::NFileStore::NStorage {

using namespace NMonitoring;

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TIndexTabletTest)
{
    Y_UNIT_TEST(ShouldBootTablet)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");
        ui64 tabletId = env.BootIndexTablet(nodeIdx);

        TIndexTabletClient tablet(env.GetRuntime(), nodeIdx, tabletId);
        tablet.WaitReady();
    }

    Y_UNIT_TEST(ShouldUpdateConfig)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");
        ui64 tabletId = env.BootIndexTablet(nodeIdx);

        TDynamicCountersPtr counters = new TDynamicCounters();
        InitCriticalEventsCounter(counters);

        auto tabletUpdateConfigCounter = counters->GetCounter(
            "AppCriticalEvents/TabletUpdateConfigError",
            true);

        TIndexTabletClient tablet(env.GetRuntime(), nodeIdx, tabletId);

        tablet.UpdateConfig({ .CloudId = "xxx" });
        UNIT_ASSERT_VALUES_EQUAL(0, tabletUpdateConfigCounter->Val());

        constexpr ui32 newChannelCount = DefaultChannelCount + 4;
        tablet.UpdateConfig({ 
            .CloudId = "xxx",
            .ChannelCount = newChannelCount,
            .StorageMediaKind = NCloud::NProto::EStorageMediaKind::STORAGE_MEDIA_SSD});
        UNIT_ASSERT_VALUES_EQUAL(0, tabletUpdateConfigCounter->Val());
        {
            auto stats = GetStorageStats(tablet);
            UNIT_ASSERT_VALUES_EQUAL(
                newChannelCount,
                stats.GetConfigChannelCount());
            
            UNIT_ASSERT_EQUAL(
                NCloud::NProto::EStorageMediaKind::STORAGE_MEDIA_SSD,
                GetStorageMediaKind(tablet));
        }
    }

    Y_UNIT_TEST(ShouldUpdatePerformanceProfileConfig)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        const auto nodeIdx = env.CreateNode("nfs");
        const auto tabletId = env.BootIndexTablet(nodeIdx);

        const auto counters = new TDynamicCounters();
        InitCriticalEventsCounter(counters);

        const auto tabletUpdateConfigCounter = counters->GetCounter(
            "AppCriticalEvents/TabletUpdateConfigError",
            true);

        TIndexTabletClient tablet(env.GetRuntime(), nodeIdx, tabletId);

        constexpr ui32 maxReadIops = 10;
        constexpr ui32 maxWriteIops = 15;
        constexpr ui32 maxReadBandwidth = 20;
        constexpr ui32 maxWriteBandwidth = 25;
        tablet.UpdateConfig({ 
            .FileSystemId = "test_filesystem",
            .CloudId = "test_cloud",
            .FolderId = "test_folder",
            .PerformanceProfile = {
                .MaxReadIops = maxReadIops,
                .MaxWriteIops = maxWriteIops,
                .MaxReadBandwidth = maxReadBandwidth,
                .MaxWriteBandwidth = maxWriteBandwidth
            }});

        auto config = GetFileSystemConfig(tablet);
        UNIT_ASSERT_VALUES_EQUAL(
            maxReadIops,
            config.GetPerformanceProfile().GetMaxReadIops());
        UNIT_ASSERT_VALUES_EQUAL(
            maxWriteIops,
            config.GetPerformanceProfile().GetMaxWriteIops());
        UNIT_ASSERT_VALUES_EQUAL(
            maxReadBandwidth,
            config.GetPerformanceProfile().GetMaxReadBandwidth());
        UNIT_ASSERT_VALUES_EQUAL(
            maxWriteBandwidth,
            config.GetPerformanceProfile().GetMaxWriteBandwidth());
    }

    Y_UNIT_TEST(ShouldFailToUpdateConfigIfValidationFails)
    {
        constexpr ui32 channelCount = 6;

        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");
        ui64 tabletId = env.BootIndexTablet(nodeIdx);

        TDynamicCountersPtr counters = new TDynamicCounters();
        InitCriticalEventsCounter(counters);

        auto tabletUpdateConfigCounter = counters->GetCounter(
            "AppCriticalEvents/TabletUpdateConfigError",
            true);

        TIndexTabletClient tablet(env.GetRuntime(), nodeIdx, tabletId, {
            .ChannelCount = channelCount
        });

        // Should return OK status due to schemeshard exotic behaviour.
        tablet.UpdateConfig({
            .BlockSize = 4 * 4096, .ChannelCount = channelCount
        });
        UNIT_ASSERT_VALUES_EQUAL(1, tabletUpdateConfigCounter->Val());

        tablet.UpdateConfig({
            .BlockCount = 1, .ChannelCount = channelCount
        });
        UNIT_ASSERT_VALUES_EQUAL(2, tabletUpdateConfigCounter->Val());

        tablet.UpdateConfig({
            .ChannelCount = channelCount - 1
        });
        UNIT_ASSERT_VALUES_EQUAL(3, tabletUpdateConfigCounter->Val());
        {
            auto stats = GetStorageStats(tablet);
            UNIT_ASSERT_VALUES_EQUAL(
                channelCount,
                stats.GetConfigChannelCount());
        }
    }
}

}   // namespace NCloud::NFileStore::NStorage
