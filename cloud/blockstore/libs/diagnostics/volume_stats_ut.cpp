#include "volume_stats.h"

#include "config.h"

#include <cloud/blockstore/libs/common/leaky_bucket.h>

#include <cloud/storage/core/libs/common/timer.h>
#include <cloud/storage/core/libs/diagnostics/monitoring.h>

#include <library/cpp/monlib/dynamic_counters/counters.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/datetime/cputimer.h>

namespace NCloud::NBlockStore {

////////////////////////////////////////////////////////////////////////////////

namespace {

void Mount(
    IVolumeStatsPtr volumeStats,
    const TString& name,
    const TString& client,
    const TString& instance,
    NCloud::NProto::EStorageMediaKind mediaKind)
{
    NProto::TVolume volume;
    volume.SetDiskId(name);
    volume.SetStorageMediaKind(mediaKind);
    volume.SetBlockSize(DefaultBlockSize);

    volumeStats->MountVolume(volume, client, instance);
}

struct TTestTimer
    : public ITimer
{
    TInstant CurrentTime;

    TInstant Now() override
    {
        return CurrentTime;
    }
};


}   // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TVolumeStatsTest)
{
    Y_UNIT_TEST(ShouldTrackRequestsPerVolume)
    {
        auto monitoring = CreateMonitoringServiceStub();
        auto counters = monitoring
            ->GetCounters()
            ->GetSubgroup("counters", "blockstore")
            ->GetSubgroup("component", "server_volume");

        auto volumeStats = CreateVolumeStats(
            monitoring,
            {},
            EVolumeStatsType::EServerStats,
            CreateWallClockTimer());

        auto getCounters = [&] (auto volume, auto instance) {
            return counters
                ->GetSubgroup("host", "cluster")
                ->GetSubgroup("volume", volume)
                ->GetSubgroup("instance", instance);
        };

        auto writeData = [](auto volume, auto type){
            auto started = volume->RequestStarted(
                type,
                1024 * 1024);

            volume->RequestCompleted(
                type,
                started,
                TDuration::Zero(),
                1024 * 1024,
                EErrorKind::Success);
        };

        auto readData = [](auto volume, auto type){
            auto started = volume->RequestStarted(
                type,
                1024 * 1024);

            volume->RequestCompleted(
                type,
                started,
                TDuration::Zero(),
                1024 * 1024,
                EErrorKind::Success);
        };

        Mount(
            volumeStats,
            "test1",
            "client1",
            "instance1",
            NCloud::NProto::STORAGE_MEDIA_SSD);
        Mount(
            volumeStats,
            "test2",
            "client2",
            "instance1",
            NCloud::NProto::STORAGE_MEDIA_HDD);
        Mount(
            volumeStats,
            "test2",
            "client3",
            "instance2",
            NCloud::NProto::STORAGE_MEDIA_HDD);

        auto volume1 = volumeStats->GetVolumeInfo("test1", "client1");
        auto volume2 = volumeStats->GetVolumeInfo("test2", "client2");
        auto volume3 = volumeStats->GetVolumeInfo("test2", "client3");

        auto volume1Counters = getCounters("test1", "instance1");
        auto volume1WriteCount = volume1Counters
            ->GetSubgroup("request", "WriteBlocks")
            ->GetCounter("Count");
        auto volume1ReadCount = volume1Counters
            ->GetSubgroup("request", "ReadBlocks")
            ->GetCounter("Count");

        auto volume2Counters = getCounters("test2", "instance1");
        auto volume2WriteCount = volume2Counters
            ->GetSubgroup("request", "WriteBlocks")
            ->GetCounter("Count");

        auto volume3Counters = getCounters("test2", "instance2");
        auto volume3WriteCount = volume3Counters
            ->GetSubgroup("request", "WriteBlocks")
            ->GetCounter("Count");

        UNIT_ASSERT_EQUAL(volume1WriteCount->Val(), 0);
        UNIT_ASSERT_EQUAL(volume1ReadCount->Val(), 0);
        UNIT_ASSERT_EQUAL(volume2WriteCount->Val(), 0);
        UNIT_ASSERT_EQUAL(volume3WriteCount->Val(), 0);

        writeData(volume1, EBlockStoreRequest::WriteBlocks);

        UNIT_ASSERT_EQUAL(volume1WriteCount->Val(), 1);
        UNIT_ASSERT_EQUAL(volume1ReadCount->Val(), 0);
        UNIT_ASSERT_EQUAL(volume2WriteCount->Val(), 0);
        UNIT_ASSERT_EQUAL(volume3WriteCount->Val(), 0);

        writeData(volume2, EBlockStoreRequest::WriteBlocks);
        readData(volume1, EBlockStoreRequest::ReadBlocks);

        UNIT_ASSERT_EQUAL(volume1WriteCount->Val(), 1);
        UNIT_ASSERT_EQUAL(volume1ReadCount->Val(), 1);
        UNIT_ASSERT_EQUAL(volume2WriteCount->Val(), 1);
        UNIT_ASSERT_EQUAL(volume3WriteCount->Val(), 0);

        writeData(volume3, EBlockStoreRequest::WriteBlocks);

        UNIT_ASSERT_EQUAL(volume1WriteCount->Val(), 1);
        UNIT_ASSERT_EQUAL(volume1ReadCount->Val(), 1);
        UNIT_ASSERT_EQUAL(volume2WriteCount->Val(), 1);
        UNIT_ASSERT_EQUAL(volume3WriteCount->Val(), 1);

        writeData(volume1, EBlockStoreRequest::WriteBlocksLocal);
        readData(volume1, EBlockStoreRequest::ReadBlocksLocal);

        UNIT_ASSERT_EQUAL(volume1WriteCount->Val(), 2);
        UNIT_ASSERT_EQUAL(volume1ReadCount->Val(), 2);
        UNIT_ASSERT_EQUAL(volume2WriteCount->Val(), 1);
        UNIT_ASSERT_EQUAL(volume3WriteCount->Val(), 1);
    }

    Y_UNIT_TEST(ShouldRegisterAndUnregisterCountersPerVolume)
    {
        auto inactivityTimeout = TDuration::MilliSeconds(10);

        auto monitoring = CreateMonitoringServiceStub();
        auto counters = monitoring
            ->GetCounters()
            ->GetSubgroup("counters", "blockstore")
            ->GetSubgroup("component", "server_volume");

        std::shared_ptr<TTestTimer> Timer = std::make_shared<TTestTimer>();
        Timer->CurrentTime = TInstant::Now();

        auto volumeStats = CreateVolumeStats(
            monitoring,
            inactivityTimeout,
            EVolumeStatsType::EServerStats,
            Timer);

        Mount(
            volumeStats,
            "test1",
            "client1",
            "instance1",
            NCloud::NProto::STORAGE_MEDIA_SSD);

        Mount(
            volumeStats,
            "test2",
            "client2",
            "instance2",
            NCloud::NProto::STORAGE_MEDIA_SSD);

        UNIT_ASSERT(counters
            ->GetSubgroup("host", "cluster")
            ->GetSubgroup("volume", "test1")
            ->FindSubgroup("instance", "instance1"));

        UNIT_ASSERT(counters
            ->GetSubgroup("host", "cluster")
            ->GetSubgroup("volume", "test2")
            ->FindSubgroup("instance", "instance2"));

        Timer->CurrentTime += inactivityTimeout * 0.5;
        volumeStats->TrimVolumes();

        Mount(
            volumeStats,
            "test2",
            "client3",
            "instance1",
            NCloud::NProto::STORAGE_MEDIA_SSD);

        UNIT_ASSERT(counters
            ->GetSubgroup("host", "cluster")
            ->GetSubgroup("volume", "test1")
            ->FindSubgroup("instance", "instance1"));

        UNIT_ASSERT(counters
            ->GetSubgroup("host", "cluster")
            ->GetSubgroup("volume", "test2")
            ->FindSubgroup("instance", "instance2"));

        UNIT_ASSERT(counters
            ->GetSubgroup("host", "cluster")
            ->GetSubgroup("volume", "test2")
            ->FindSubgroup("instance", "instance1"));

        Timer->CurrentTime += inactivityTimeout * 0.6;
        volumeStats->TrimVolumes();

        UNIT_ASSERT(!counters
            ->GetSubgroup("host", "cluster")
            ->GetSubgroup("volume", "test1")
            ->FindSubgroup("instance", "instance1"));

        UNIT_ASSERT(!counters
            ->GetSubgroup("host", "cluster")
            ->GetSubgroup("volume", "test2")
            ->FindSubgroup("instance", "instance2"));

        UNIT_ASSERT(counters
            ->GetSubgroup("host", "cluster")
            ->GetSubgroup("volume", "test2")
            ->FindSubgroup("instance", "instance1"));
    }

    Y_UNIT_TEST(ShouldTrackSilentErrorsPerVolume)
    {
        auto monitoring = CreateMonitoringServiceStub();
        auto counters = monitoring
            ->GetCounters()
            ->GetSubgroup("counters", "blockstore")
            ->GetSubgroup("component", "server_volume");

        auto volumeStats = CreateVolumeStats(
            monitoring,
            {},
            EVolumeStatsType::EServerStats,
            CreateWallClockTimer());

        auto getCounters = [&] (auto volume, auto instance) {
            auto volumeCounters = counters
                ->GetSubgroup("host", "cluster")
                ->GetSubgroup("volume", volume)
                ->GetSubgroup("instance", instance)
                ->GetSubgroup("request", "WriteBlocks");

            return std::make_pair(
                volumeCounters->GetCounter("Errors/Fatal"),
                volumeCounters->GetCounter("Errors/Silent")
            );
        };

        auto shoot = [] (auto volume, auto errorKind) {
            auto started = volume->RequestStarted(
                EBlockStoreRequest::WriteBlocks,
                1024 * 1024);

            volume->RequestCompleted(
                EBlockStoreRequest::WriteBlocks,
                started,
                TDuration::Zero(),
                1024 * 1024,
                errorKind);
        };

        Mount(
            volumeStats,
            "test1",
            "client1",
            "instance1",
            NCloud::NProto::STORAGE_MEDIA_SSD);
        Mount(
            volumeStats,
            "test2",
            "client2",
            "instance2",
            NCloud::NProto::STORAGE_MEDIA_HDD);

        auto volume1 = volumeStats->GetVolumeInfo("test1", "client1");
        auto volume2 = volumeStats->GetVolumeInfo("test2", "client2");

        auto [volume1Errors, volume1Silent] = getCounters("test1", "instance1");
        auto [volume2Errors, volume2Silent] = getCounters("test2", "instance2");

        UNIT_ASSERT_VALUES_EQUAL(0, volume1Errors->Val());
        UNIT_ASSERT_VALUES_EQUAL(0, volume1Silent->Val());
        UNIT_ASSERT_VALUES_EQUAL(0, volume2Errors->Val());
        UNIT_ASSERT_VALUES_EQUAL(0, volume2Silent->Val());

        shoot(volume1, EErrorKind::ErrorFatal);

        UNIT_ASSERT_VALUES_EQUAL(1, volume1Errors->Val());
        UNIT_ASSERT_VALUES_EQUAL(0, volume1Silent->Val());
        UNIT_ASSERT_VALUES_EQUAL(0, volume2Errors->Val());
        UNIT_ASSERT_VALUES_EQUAL(0, volume2Silent->Val());

        shoot(volume2, EErrorKind::ErrorFatal);

        UNIT_ASSERT_VALUES_EQUAL(1, volume1Errors->Val());
        UNIT_ASSERT_VALUES_EQUAL(0, volume1Silent->Val());
        UNIT_ASSERT_VALUES_EQUAL(1, volume2Errors->Val());
        UNIT_ASSERT_VALUES_EQUAL(0, volume2Silent->Val());

        shoot(volume1, EErrorKind::ErrorSilent);

        UNIT_ASSERT_VALUES_EQUAL(1, volume1Errors->Val());
        UNIT_ASSERT_VALUES_EQUAL(1, volume1Silent->Val());
        UNIT_ASSERT_VALUES_EQUAL(1, volume2Errors->Val());
        UNIT_ASSERT_VALUES_EQUAL(0, volume2Silent->Val());

        shoot(volume2, EErrorKind::ErrorSilent);

        UNIT_ASSERT_VALUES_EQUAL(1, volume1Errors->Val());
        UNIT_ASSERT_VALUES_EQUAL(1, volume1Silent->Val());
        UNIT_ASSERT_VALUES_EQUAL(1, volume2Errors->Val());
        UNIT_ASSERT_VALUES_EQUAL(1, volume2Silent->Val());
    }

    Y_UNIT_TEST(ShouldReportSufferMetrics)
    {
        auto inactivityTimeout = TDuration::MilliSeconds(10);

        auto monitoring = CreateMonitoringServiceStub();
        auto counters = monitoring
            ->GetCounters()
            ->GetSubgroup("counters", "blockstore")
            ->GetSubgroup("component", "server");

        NProto::TDiagnosticsConfig cfg;
        cfg.MutableSsdPerfSettings()->MutableWrite()->SetIops(4200);
        cfg.MutableSsdPerfSettings()->MutableWrite()->SetBandwidth(342000000);
        cfg.MutableSsdPerfSettings()->MutableRead()->SetIops(4200);
        cfg.MutableSsdPerfSettings()->MutableRead()->SetBandwidth(342000000);
        auto diagConfig = std::make_shared<TDiagnosticsConfig>(std::move(cfg));

        auto volumeStats = CreateVolumeStats(
            monitoring,
            diagConfig,
            inactivityTimeout,
            EVolumeStatsType::EServerStats,
            CreateWallClockTimer());

        Mount(
            volumeStats,
            "test1",
            "client1",
            "instance1",
            NCloud::NProto::STORAGE_MEDIA_SSD);

        auto volume = volumeStats->GetVolumeInfo("test1", "client1");

        auto requestDuration = TDuration::MilliSeconds(400) +
            diagConfig->GetExpectedIoParallelism() * CostPerIO(
                diagConfig->GetSsdPerfSettings().WriteIops,
                diagConfig->GetSsdPerfSettings().WriteBandwidth,
                1024 * 1024);
        auto durationInCycles = DurationToCyclesSafe(requestDuration);
        auto now = GetCycleCount();

        volume->RequestCompleted(
            EBlockStoreRequest::WriteBlocks,
            now - Min(now, durationInCycles),
            {},
            1024 * 1024,
            {});

        volumeStats->UpdateStats(false);

        auto sufferArray = volumeStats->GatherVolumePerfStatuses();
        UNIT_ASSERT_VALUES_EQUAL(1, sufferArray.size());
        UNIT_ASSERT_VALUES_EQUAL("test1", sufferArray[0].first);
        UNIT_ASSERT_VALUES_EQUAL(1, sufferArray[0].second);

        UNIT_ASSERT_VALUES_EQUAL(1, counters->GetCounter("DisksSuffer", false)->Val());
        UNIT_ASSERT_VALUES_EQUAL(
            1,
            counters->GetSubgroup("type", "ssd")->GetCounter("DisksSuffer", false)->Val());
        UNIT_ASSERT_VALUES_EQUAL(
            0,
            counters->GetSubgroup("type", "hdd")->GetCounter("DisksSuffer", false)->Val());
    }
}

}   // namespace NCloud::NBlockStore
