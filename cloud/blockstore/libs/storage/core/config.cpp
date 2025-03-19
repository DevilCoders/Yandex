#include "config.h"

#include "features_config.h"

#include <ydb/core/control/immediate_control_board_impl.h>

#include <library/cpp/monlib/service/pages/templates.h>

#include <util/generic/size_literals.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NKikimr;

namespace {

////////////////////////////////////////////////////////////////////////////////

TDuration Days(ui32 value)
{
    return TDuration::Days(value);
}

TDuration Hours(ui32 value)
{
    return TDuration::Hours(value);
}

TDuration Minutes(ui32 value)
{
    return TDuration::Minutes(value);
}

TDuration Seconds(ui32 value)
{
    return TDuration::Seconds(value);
}

TDuration MSeconds(ui32 value)
{
    return TDuration::MilliSeconds(value);
}

////////////////////////////////////////////////////////////////////////////////

#define BLOCKSTORE_STORAGE_CONFIG_RO(xxx)                                      \
    xxx(SchemeShardDir,                TString,   "/Root"                     )\
    xxx(MeteringFilename,              TString,   ""                          )\
    xxx(DisableLocalService,           bool,      false                       )\
    xxx(ServiceVersionInfo,            TString,   ""                          )\
    xxx(FolderId,                      TString,   ""                          )\
                                                                               \
    xxx(HDDSystemChannelPoolKind,      TString,      "rot"                    )\
    xxx(HDDLogChannelPoolKind,         TString,      "rot"                    )\
    xxx(HDDIndexChannelPoolKind,       TString,      "rot"                    )\
    xxx(HDDMixedChannelPoolKind,       TString,      "rot"                    )\
    xxx(HDDMergedChannelPoolKind,      TString,      "rot"                    )\
    xxx(HDDFreshChannelPoolKind,       TString,      "rot"                    )\
    xxx(SSDSystemChannelPoolKind,      TString,      "ssd"                    )\
    xxx(SSDLogChannelPoolKind,         TString,      "ssd"                    )\
    xxx(SSDIndexChannelPoolKind,       TString,      "ssd"                    )\
    xxx(SSDMixedChannelPoolKind,       TString,      "ssd"                    )\
    xxx(SSDMergedChannelPoolKind,      TString,      "ssd"                    )\
    xxx(SSDFreshChannelPoolKind,       TString,      "ssd"                    )\
    xxx(HybridSystemChannelPoolKind,   TString,      "ssd"                    )\
    xxx(HybridLogChannelPoolKind,      TString,      "ssd"                    )\
    xxx(HybridIndexChannelPoolKind,    TString,      "ssd"                    )\
    xxx(HybridMixedChannelPoolKind,    TString,      "ssd"                    )\
    xxx(HybridMergedChannelPoolKind,   TString,      "rot"                    )\
    xxx(HybridFreshChannelPoolKind,    TString,      "ssd"                    )\
                                                                               \
    /* NBS-2765 */                                                             \
    xxx(CommonSSDPoolKind,             TString,      "ssd"                    )\
    xxx(MaxSSDGroupWriteIops,          ui64,         40000                    )\
    xxx(MaxSSDGroupWriteBandwidth,     ui64,         424 * 1024 * 1024        )\
    xxx(MaxSSDGroupReadIops,           ui64,         12496                    )\
    xxx(MaxSSDGroupReadBandwidth,      ui64,         450 * 1024 * 1024        )\
    xxx(CommonHDDPoolKind,             TString,      "rot"                    )\
    xxx(MaxHDDGroupWriteIops,          ui64,         11000                    )\
    xxx(MaxHDDGroupWriteBandwidth,     ui64,         44 * 1024 * 1024         )\
    xxx(MaxHDDGroupReadIops,           ui64,         292                      )\
    xxx(MaxHDDGroupReadBandwidth,      ui64,         66 * 1024 * 1024         )\
    xxx(CommonOverlayPrefixPoolKind,   TString,      "overlay"                )\
                                                                               \
    xxx(TabletBootInfoCacheFilePath,   TString,      ""                       )\
    xxx(PathDescriptionCacheFilePath,  TString,      ""                       )\

// BLOCKSTORE_STORAGE_CONFIG_RO

#define BLOCKSTORE_STORAGE_CONFIG_RW(xxx)                                      \
    xxx(WriteBlobThreshold,            ui32,      1_MB                        )\
    xxx(WriteBlobThresholdSSD,         ui32,      128_KB                      )\
    xxx(WriteMergedBlobThreshold,      ui32,      3_MB                        )\
    xxx(FlushThreshold,                ui32,      4_MB                        )\
    xxx(FreshBlobCountFlushThreshold,  ui32,      3200                        )\
    xxx(FreshBlobByteCountFlushThreshold,   ui32,      16_MB                  )\
                                                                               \
    xxx(SSDCompactionType,                                                     \
            NProto::ECompactionType,                                           \
            NProto::CT_DEFAULT                                                )\
    xxx(HDDCompactionType,                                                     \
            NProto::ECompactionType,                                           \
            NProto::CT_DEFAULT                                                )\
    xxx(CompactionThreshold,                ui32,      5                      )\
    xxx(CompactionGarbageThreshold,         ui32,      20                     )\
    xxx(CompactionGarbageBlobLimit,         ui32,      100                    )\
    xxx(CompactionGarbageBlockLimit,        ui32,      10240                  )\
    xxx(CompactionRangeGarbageThreshold,    ui32,      200                    )\
    xxx(MaxAffectedBlocksPerCompaction,     ui32,      8192                   )\
    xxx(V1GarbageCompactionEnabled,         bool,      false                  )\
    xxx(OptimizeForShortRanges,             bool,      false                  )\
    xxx(MaxCompactionDelay,                 TDuration, TDuration::Zero()      )\
    xxx(MinCompactionDelay,                 TDuration, TDuration::Zero()      )\
    xxx(MaxCompactionExecTimePerSecond,     TDuration, TDuration::Zero()      )\
    xxx(CompactionScoreHistorySize,             ui32,   10                    )\
    xxx(CompactionScoreLimitForThrottling,      ui32,   300                   )\
    xxx(TargetCompactionBytesPerOp,             ui64,   64_KB                 )\
    xxx(MaxSkippedBlobsDuringCompaction,        ui32,   3                     )\
    xxx(IncrementalCompactionEnabled,           bool,   false                 )\
    xxx(CompactionRangeCountPerRun,             ui32,   10                    )\
    xxx(BatchCompactionEnabled,                 bool,   false                 )\
                                                                               \
    xxx(CleanupThreshold,                       ui32,      10                 )\
    xxx(MaxCleanupDelay,                        TDuration, TDuration::Zero()  )\
    xxx(MinCleanupDelay,                        TDuration, TDuration::Zero()  )\
    xxx(MaxCleanupExecTimePerSecond,            TDuration, TDuration::Zero()  )\
    xxx(CleanupScoreHistorySize,                ui32,      10                 )\
    xxx(CleanupQueueBytesLimitForThrottling,    ui64,      100_MB             )\
    /* measured in overwritten blocks */                                       \
    xxx(UpdateBlobsThreshold,                   ui32,      100                )\
    xxx(MaxBlobsToCleanup,                      ui32,      100                )\
                                                                               \
    xxx(CollectGarbageThreshold,       ui32,      10                          )\
    xxx(RunV2SoftGcAtStartup,                   bool,      false              )\
    xxx(DontEnqueueCollectGarbageUponPartitionStartup,  bool,      false      )\
    xxx(HiveLockExpireTimeout,         TDuration, Seconds(30)                 )\
    xxx(TabletRebootCoolDownIncrement, TDuration, MSeconds(500)               )\
    xxx(TabletRebootCoolDownMax,       TDuration, Seconds(30)                 )\
    xxx(PipeClientRetryCount,          ui32,      4                           )\
    xxx(PipeClientMinRetryTime,        TDuration, Seconds(1)                  )\
    xxx(PipeClientMaxRetryTime,        TDuration, Seconds(4)                  )\
    xxx(FlushBlobSizeThreshold,        ui32,      4_MB                        )\
    xxx(FlushToDevNull,                bool,      false                       )\
    xxx(UsageStatsLogPeriod,           TDuration, Seconds(60)                 )\
    xxx(CompactionRetryTimeout,        TDuration, Seconds(1)                  )\
    xxx(CleanupRetryTimeout,           TDuration, Seconds(1)                  )\
    xxx(MaxReadWriteRangeSize,         ui64,      4_GB                        )\
    xxx(MaxBlobRangeSize,              ui32,      128_MB                      )\
    xxx(MaxRangesPerBlob,              ui32,      8                           )\
    xxx(MaxBlobSize,                   ui32,      4_MB                        )\
    xxx(InactiveClientsTimeout,        TDuration, Seconds(9)                  )\
    xxx(ClientRemountPeriod,           TDuration, Seconds(4)                  )\
    xxx(InitialAddClientTimeout,       TDuration, Seconds(1)                  )\
    xxx(LocalStartAddClientTimeout,    TDuration, Minutes(1)                  )\
    xxx(MaxIORequestsInFlight,         ui32,      10                          )\
    xxx(MaxIORequestsInFlightSSD,      ui32,      1000                        )\
    xxx(AllowVersionInModifyScheme,    bool,      false                       )\
                                                                               \
    xxx(AllocationUnitSSD,                  ui32,      32                     )\
    xxx(SSDUnitReadBandwidth,               ui32,      15                     )\
    xxx(SSDUnitWriteBandwidth,              ui32,      15                     )\
    xxx(SSDMaxReadBandwidth,                ui32,      450                    )\
    xxx(SSDMaxWriteBandwidth,               ui32,      450                    )\
    xxx(RealSSDUnitReadBandwidth,           ui32,      15                     )\
    xxx(RealSSDUnitWriteBandwidth,          ui32,      15                     )\
    xxx(SSDUnitReadIops,                    ui32,      1000                   )\
    xxx(SSDUnitWriteIops,                   ui32,      1000                   )\
    xxx(SSDMaxReadIops,                     ui32,      20000                  )\
    xxx(SSDMaxWriteIops,                    ui32,      40000                  )\
    xxx(RealSSDUnitReadIops,                ui32,      400                    )\
    xxx(RealSSDUnitWriteIops,               ui32,      1000                   )\
    /* 16000 ReadBlob requests per sec utilize all our cpus
     * We need cpu for at least 2 fully utilized disks
     * CompactionRangeSize / (MaxBandwidth / BlockSize / 8000) = 70
     */                                                                        \
    xxx(SSDMaxBlobsPerRange,                ui32,      70                     )\
    xxx(SSDV2MaxBlobsPerRange,              ui32,      20                     )\
                                                                               \
    xxx(AllocationUnitHDD,                  ui32,      256                    )\
    xxx(HDDUnitReadBandwidth,               ui32,      30                     )\
    xxx(HDDUnitWriteBandwidth,              ui32,      30                     )\
    xxx(HDDMaxReadBandwidth,                ui32,      240                    )\
    xxx(HDDMaxWriteBandwidth,               ui32,      240                    )\
    xxx(RealHDDUnitReadBandwidth,           ui32,      2                      )\
    xxx(RealHDDUnitWriteBandwidth,          ui32,      1                      )\
    xxx(HDDUnitReadIops,                    ui32,      300                    )\
    xxx(HDDUnitWriteIops,                   ui32,      300                    )\
    xxx(HDDMaxReadIops,                     ui32,      2000                   )\
    xxx(HDDMaxWriteIops,                    ui32,      11000                  )\
    xxx(RealHDDUnitReadIops,                ui32,      10                     )\
    xxx(RealHDDUnitWriteIops,               ui32,      300                    )\
    /* TODO: properly calculate def value for this param for network-hdd disks
     */                                                                        \
    xxx(HDDMaxBlobsPerRange,                ui32,      70                     )\
    xxx(HDDV2MaxBlobsPerRange,              ui32,      20                     )\
                                                                               \
    xxx(AllocationUnitNonReplicatedSSD,     ui32,      93                     )\
    xxx(NonReplicatedSSDUnitReadBandwidth,  ui32,      110                    )\
    xxx(NonReplicatedSSDUnitWriteBandwidth, ui32,      82                     )\
    xxx(NonReplicatedSSDMaxReadBandwidth,   ui32,      1024                   )\
    xxx(NonReplicatedSSDMaxWriteBandwidth,  ui32,      1024                   )\
    xxx(NonReplicatedSSDUnitReadIops,       ui32,      28000                  )\
    xxx(NonReplicatedSSDUnitWriteIops,      ui32,      5600                   )\
    xxx(NonReplicatedSSDMaxReadIops,        ui32,      75000                  )\
    xxx(NonReplicatedSSDMaxWriteIops,       ui32,      75000                  )\
                                                                               \
    xxx(AllocationUnitMirror2SSD,      ui32,      93                          )\
    xxx(Mirror2SSDUnitReadBandwidth,   ui32,      164                         )\
    xxx(Mirror2SSDUnitWriteBandwidth,  ui32,      82                          )\
    xxx(Mirror2SSDMaxReadBandwidth,    ui32,      2048                        )\
    xxx(Mirror2SSDMaxWriteBandwidth,   ui32,      1024                        )\
    xxx(Mirror2SSDUnitReadIops,        ui32,      10000                       )\
    xxx(Mirror2SSDUnitWriteIops,       ui32,      5000                        )\
    xxx(Mirror2SSDMaxReadIops,         ui32,      80000                       )\
    xxx(Mirror2SSDMaxWriteIops,        ui32,      40000                       )\
                                                                               \
    xxx(Mirror2DiskReplicaCount,       ui32,      1                           )\
                                                                               \
    xxx(AllocationUnitMirror3SSD,      ui32,      93                          )\
    xxx(Mirror3SSDUnitReadBandwidth,   ui32,      164                         )\
    xxx(Mirror3SSDUnitWriteBandwidth,  ui32,      82                          )\
    xxx(Mirror3SSDMaxReadBandwidth,    ui32,      2048                        )\
    xxx(Mirror3SSDMaxWriteBandwidth,   ui32,      1024                        )\
    xxx(Mirror3SSDUnitReadIops,        ui32,      10000                       )\
    xxx(Mirror3SSDUnitWriteIops,       ui32,      5000                        )\
    xxx(Mirror3SSDMaxReadIops,         ui32,      80000                       )\
    xxx(Mirror3SSDMaxWriteIops,        ui32,      40000                       )\
                                                                               \
    xxx(Mirror3DiskReplicaCount,       ui32,      2                           )\
                                                                               \
    xxx(ThrottlingEnabled,             bool,      false                       )\
    xxx(ThrottlingEnabledSSD,          bool,      false                       )\
    xxx(ThrottlingBurstPercentage,     ui32,      10                          )\
    xxx(ThrottlingMaxPostponedWeight,  ui32,      128_MB                      )\
    xxx(DefaultPostponedRequestWeight, ui32,      1_KB                        )\
    xxx(ThrottlingBoostTime,           TDuration, Minutes(30)                 )\
    xxx(ThrottlingBoostRefillTime,     TDuration, Hours(12)                   )\
    xxx(ThrottlingSSDBoostUnits,       ui32,      0                           )\
    xxx(ThrottlingHDDBoostUnits,       ui32,      4                           )\
    xxx(ThrottlerStateWriteInterval,   TDuration, Seconds(60)                 )\
                                                                               \
    xxx(StatsUploadInterval,           TDuration, Seconds(0)                  )\
                                                                               \
    xxx(AuthorizationMode,                                                     \
            NProto::EAuthorizationMode,                                        \
            NProto::AUTHORIZATION_IGNORE                                      )\
                                                                               \
    xxx(MaxThrottlerDelay,             TDuration, Seconds(25)                 )\
                                                                               \
    xxx(CompactionScoreLimitForBackpressure,            ui32,   300           )\
    xxx(CompactionScoreThresholdForBackpressure,        ui32,   100           )\
    xxx(CompactionScoreFeatureMaxValue,                 ui32,   10            )\
                                                                               \
    xxx(FreshByteCountLimitForBackpressure,             ui32,   64_MB         )\
    xxx(FreshByteCountThresholdForBackpressure,         ui32,   20_MB         )\
    xxx(FreshByteCountFeatureMaxValue,                  ui32,   10            )\
                                                                               \
    xxx(MaxWriteCostMultiplier,                         ui32,   10            )\
    xxx(ChannelFreeSpaceThreshold,                      ui32,   25            )\
    xxx(ChannelMinFreeSpace,                            ui32,   15            )\
    xxx(MinChannelCount,                                ui32,   4             )\
    xxx(FreshChannelCount,                              ui32,   0             )\
                                                                               \
    xxx(ZoneBlockCount,                            ui32,   32 * MaxBlocksCount)\
    xxx(HotZoneRequestCountFactor,                 ui32,   10                 )\
    xxx(ColdZoneRequestCountFactor,                ui32,   5                  )\
    xxx(BlockListCacheSizePercentage,              ui32,   100                )\
                                                                               \
    xxx(WriteRequestBatchingEnabled,               bool,      false           )\
                                                                               \
    xxx(FreshChannelWriteRequestsEnabled,          bool,      false           )\
                                                                               \
    xxx(AllocateSeparateMixedChannels,                  bool,   false         )\
    xxx(PoolKindChangeAllowed,                          bool,   false         )\
                                                                               \
    xxx(BlockDigestsEnabled,                            bool,   false         )\
    xxx(UseTestBlockDigestGenerator,                    bool,   false         )\
    xxx(DigestedBlocksPercentage,                       ui32,   1             )\
                                                                               \
    xxx(IndexStructuresConversionAttemptInterval,   TDuration,  Seconds(10)   )\
                                                                               \
    xxx(NonReplicatedDiskRecyclingPeriod,           TDuration,  Minutes(10)   )\
    xxx(NonReplicatedDiskRepairTimeout,             TDuration,  Minutes(10)   )\
    xxx(NonReplicatedAgentTimeout,                  TDuration,  Minutes(5)    )\
    xxx(AgentRequestTimeout,                        TDuration,  Seconds(1)    )\
    xxx(AcquireNonReplicatedDevices,                bool,       false         )\
    xxx(VolumePreemptionType,                                                  \
        NProto::EVolumePreemptionType,                                         \
        NProto::PREEMPTION_NONE                                               )\
    xxx(PreemptionPushPercentage,                  ui32,     80               )\
    xxx(PreemptionPullPercentage,                  ui32,     40               )\
                                                                               \
    xxx(DefaultTabletVersion,                      ui32,     0                )\
                                                                               \
    xxx(NonReplicatedInflightLimit,                ui32,        1024          )\
    xxx(MaxTimedOutDeviceStateDuration,            TDuration,   Minutes(20)   )\
                                                                               \
    xxx(MaxDisksInPlacementGroup,                  ui32,      5               )\
    xxx(BrokenDiskDestructionDelay,                TDuration, Seconds(5)      )\
    xxx(VolumeHistoryDuration,                     TDuration, Days(7)         )\
    xxx(VolumeHistoryCacheSize,                    ui32,      100             )\
    xxx(DeletedCheckpointHistoryLifetime,          TDuration, Days(7)         )\
                                                                               \
    xxx(BytesPerPartition,                         ui64,      512_TB          )\
    xxx(BytesPerPartitionSSD,                      ui64,      512_GB          )\
    xxx(BytesPerStripe,                            ui32,      16_MB           )\
    xxx(MaxPartitionsPerVolume,                    ui32,      2               )\
    xxx(MultipartitionVolumesEnabled,              bool,      false           )\
    xxx(NonReplicatedInfraTimeout,                 TDuration, Days(1)         )\
    xxx(NonReplicatedInfraUnavailableAgentTimeout, TDuration, Hours(1)        )\
    xxx(NonReplicatedMinRequestTimeout,            TDuration, MSeconds(100)   )\
    xxx(NonReplicatedMaxRequestTimeout,            TDuration, Seconds(30)     )\
    xxx(NonReplicatedMigrationStartAllowed,        bool,      false           )\
    xxx(NonReplicatedVolumeMigrationDisabled,      bool,      false           )\
    xxx(MigrationIndexCachingInterval,             ui32,      65536           )\
    xxx(MaxMigrationBandwidth,                     ui32,      100             )\
    xxx(ExpectedDiskAgentSize,                     ui32,      15              )\
    /* 75 devices = 5 agents */                                                \
    xxx(MaxNonReplicatedDeviceMigrationsInProgress,ui32,      75              )\
    xxx(PlacementGroupAlertPeriod,                 TDuration, Hours(8)        )\
    xxx(EnableLoadActor,                           bool,      false           )\
                                                                               \
    xxx(UserDataDebugDumpAllowed,                  bool,      false           )\
    xxx(CpuMatBenchNsSystemThreshold,              ui64,      3000            )\
    xxx(CpuMatBenchNsUserThreshold,                ui64,      3000            )\
    xxx(CpuLackThreshold,                          ui32,      100             )\
    xxx(InitialPullDelay,                          TDuration, Minutes(10)     )\
                                                                               \
    xxx(LogicalUsedBlocksCalculationEnabled,       bool,      false           )\
    xxx(LogicalUsedBlocksUpdateBlockCount,         ui32,      128'000'000     )\
                                                                               \
    xxx(DumpBlockCommitIdsIntoProfileLog,          bool,      false           )\
    xxx(DumpBlobUpdatesIntoProfileLog,             bool,      false           )\
                                                                               \
    /* NBS-2451 */                                                             \
    xxx(EnableConversionIntoMixedIndexV2,          bool,      false           )\
                                                                               \
    xxx(StatsUploadDiskCount,                      ui32,      1000            )\
    xxx(StatsUploadRetryTimeout,                   TDuration, Seconds(5)      )\
                                                                               \
    xxx(RemoteMountOnly,                           bool,      false           )\
    xxx(MaxLocalVolumes,                           ui32,      100             )\
                                                                               \
    xxx(DiskRegistryVolumeConfigUpdatePeriod,      TDuration, Minutes(5)      )\
                                                                               \
    xxx(ReassignRequestRetryTimeout,               TDuration, Seconds(5)      )\
                                                                               \
    xxx(MixedIndexCacheV1Enabled,                  bool,      false           )\
    xxx(MixedIndexCacheV1SizeSSD,                  ui32,      32 * 1024       )\
                                                                               \
    xxx(MaxReadBlobErrorsBeforeSuicide,            ui32,      5               )\
    xxx(RejectMountOnAddClientTimeout,             bool,      false           )\
    xxx(NonReplicatedVolumeNotificationTimeout,    TDuration, Seconds(30)     )\
    xxx(NonReplicatedSecureEraseTimeout,           TDuration, Minutes(10)     )\
                                                                               \
    xxx(HiveProxyFallbackMode,                     bool,      false           )\
    xxx(SSProxyFallbackMode,                       bool,      false           )\
    xxx(AllocationUnitLocalSSD,                    ui32,      93              )\
                                                                               \
    xxx(RdmaTargetPort,                            ui32,      10020           )\
    xxx(UseNonreplicatedRdmaActor,                 bool,      false           )\
    xxx(UseRdma,                                   bool,      false           )\
// BLOCKSTORE_STORAGE_CONFIG_RW

#define BLOCKSTORE_STORAGE_CONFIG(xxx)                                         \
    BLOCKSTORE_STORAGE_CONFIG_RO(xxx)                                          \
    BLOCKSTORE_STORAGE_CONFIG_RW(xxx)                                          \
// BLOCKSTORE_STORAGE_CONFIG

#define BLOCKSTORE_STORAGE_DECLARE_CONFIG(name, type, value)                   \
    Y_DECLARE_UNUSED static const type Default##name = value;                  \
// BLOCKSTORE_STORAGE_DECLARE_CONFIG

BLOCKSTORE_STORAGE_CONFIG(BLOCKSTORE_STORAGE_DECLARE_CONFIG)

#undef BLOCKSTORE_STORAGE_DECLARE_CONFIG

#define BLOCKSTORE_FEATURE(xxx)                                                \
    xxx(Balancer)                                                              \
    xxx(IncrementalCompaction)                                                 \
    xxx(MultipartitionVolumes)                                                 \
    xxx(AllocateFreshChannel)                                                  \
    xxx(FreshChannelWriteRequests)                                             \
    xxx(MixedIndexCacheV1)                                                     \
    xxx(BatchCompaction)                                                       \
    xxx(UseRdma)                                                               \

// BLOCKSTORE_FEATURE

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

template <typename TTarget, typename TSource>
TTarget ConvertValue(const TSource& value)
{
    return static_cast<TTarget>(value);
}

template <>
TDuration ConvertValue<TDuration, ui64>(const ui64& value)
{
    return TDuration::MilliSeconds(value);
}

////////////////////////////////////////////////////////////////////////////////

IOutputStream& operator <<(
    IOutputStream& out,
    NProto::EAuthorizationMode mode)
{
    switch (mode) {
        case NProto::AUTHORIZATION_ACCEPT:
            return out << "AUTHORIZATION_ACCEPT";
        case NProto::AUTHORIZATION_REQUIRE:
            return out << "AUTHORIZATION_REQUIRE";
        case NProto::AUTHORIZATION_IGNORE:
            return out << "AUTHORIZATION_IGNORE";
        default:
            return out
                << "(Unknown EAuthorizationMode value "
                << (int)mode
                << ")";
    }
}

IOutputStream& operator <<(
    IOutputStream& out,
    NProto::ECompactionType ct)
{
    switch (ct) {
        case NProto::CT_DEFAULT:
            return out << "CT_DEFAULT";
        case NProto::CT_LOAD:
            return out << "CT_LOAD";
        default:
            return out
                << "(Unknown ECompactionType value "
                << (int)ct
                << ")";
    }
}

IOutputStream& operator <<(
    IOutputStream& out,
    NProto::EVolumePreemptionType pt)
{
    switch (pt) {
        case NProto::PREEMPTION_NONE:
            return out << "PREEMPTION_NONE";
        case NProto::PREEMPTION_MOVE_MOST_HEAVY:
            return out << "PREEMPTION_MOVE_MOST_HEAVY";
        case NProto::PREEMPTION_MOVE_LEAST_HEAVY:
            return out << "PREEMPTION_MOVE_LEAST_HEAVY";
        default:
            return out
                << "(Unknown EVolumePreemptionType value "
                << (int)pt
                << ")";
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

struct TStorageConfig::TImpl
{
    NProto::TStorageServiceConfig StorageServiceConfig;
    TFeaturesConfigPtr FeaturesConfig;

#define BLOCKSTORE_CONFIG_CONTROL(name, type, value)                           \
    TControlWrapper Control##name { 0, 0, Max() };                             \
// BLOCKSTORE_CONFIG_CONTROL

    BLOCKSTORE_STORAGE_CONFIG_RW(BLOCKSTORE_CONFIG_CONTROL)

#undef BLOCKSTORE_CONFIG_CONTROL

    TImpl(
            NProto::TStorageServiceConfig storageServiceConfig,
            TFeaturesConfigPtr featuresConfig)
        : StorageServiceConfig(std::move(storageServiceConfig))
        , FeaturesConfig(std::move(featuresConfig))
    {}

    void SetFeaturesConfig(TFeaturesConfigPtr featuresConfig)
    {
        FeaturesConfig = std::move(featuresConfig);
    }
};

////////////////////////////////////////////////////////////////////////////////

TStorageConfig::TStorageConfig(
    NProto::TStorageServiceConfig storageServiceConfig,
    TFeaturesConfigPtr featuresConfig)
    : Impl(new TImpl(std::move(storageServiceConfig), std::move(featuresConfig)))
{}

TStorageConfig::~TStorageConfig()
{}

void TStorageConfig::SetFeaturesConfig(TFeaturesConfigPtr featuresConfig)
{
    Impl->SetFeaturesConfig(std::move(featuresConfig));
}

void TStorageConfig::Register(TControlBoard& controlBoard)
{
#define BLOCKSTORE_CONFIG_CONTROL(name, type, value)                           \
    controlBoard.RegisterSharedControl(                                        \
        Impl->Control##name,                                                   \
        "BlockStore_" #name);                                                  \
// BLOCKSTORE_CONFIG_CONTROL

    BLOCKSTORE_STORAGE_CONFIG_RW(BLOCKSTORE_CONFIG_CONTROL)

#undef BLOCKSTORE_CONFIG_CONTROL
}

#define BLOCKSTORE_CONFIG_GETTER(name, type, ...)                              \
type TStorageConfig::Get##name() const                                         \
{                                                                              \
    auto value = Impl->StorageServiceConfig.Get##name();                       \
    return value ? ConvertValue<type>(value) : Default##name;                  \
}                                                                              \
// BLOCKSTORE_CONFIG_GETTER

BLOCKSTORE_STORAGE_CONFIG_RO(BLOCKSTORE_CONFIG_GETTER)

#undef BLOCKSTORE_CONFIG_GETTER

#define BLOCKSTORE_CONFIG_GETTER(name, type, ...)                              \
type TStorageConfig::Get##name() const                                         \
{                                                                              \
    ui64 value = Impl->Control##name;                                          \
    if (!value) value = Impl->StorageServiceConfig.Get##name();                \
    return value ? ConvertValue<type>(value) : Default##name;                  \
}                                                                              \
// BLOCKSTORE_CONFIG_GETTER

BLOCKSTORE_STORAGE_CONFIG_RW(BLOCKSTORE_CONFIG_GETTER)

#undef BLOCKSTORE_CONFIG_GETTER

void TStorageConfig::Dump(IOutputStream& out) const
{
#define BLOCKSTORE_CONFIG_DUMP(name, ...)                                      \
    out << #name << ": " << Get##name() << Endl;                               \
// BLOCKSTORE_CONFIG_DUMP

    BLOCKSTORE_STORAGE_CONFIG(BLOCKSTORE_CONFIG_DUMP);

#undef BLOCKSTORE_CONFIG_DUMP
}

void TStorageConfig::DumpHtml(IOutputStream& out) const
{
#define BLOCKSTORE_CONFIG_DUMP(name, ...)                                      \
    TABLER() {                                                                 \
        TABLED() { out << #name; }                                             \
        TABLED() { out << Get##name(); }                                       \
    }                                                                          \
// BLOCKSTORE_CONFIG_DUMP

    HTML(out) {
        TABLE_CLASS("table table-condensed") {
            TABLEBODY() {
                BLOCKSTORE_STORAGE_CONFIG(BLOCKSTORE_CONFIG_DUMP);
            }
        }
    }

#undef BLOCKSTORE_CONFIG_DUMP
}

#define BLOCKSTORE_FEATURE_GETTER(name)                                        \
bool TStorageConfig::Is##name##FeatureEnabled(                                 \
    const TString& cloudId,                                                    \
    const TString& folderId) const                                             \
{                                                                              \
    return Impl->FeaturesConfig->IsFeatureEnabled(cloudId, folderId, #name);   \
}                                                                              \

// BLOCKSTORE_FEATURE_GETTER

    BLOCKSTORE_FEATURE(BLOCKSTORE_FEATURE_GETTER)

#undef BLOCKSTORE_FEATURE_GETTER

ui64 GetAllocationUnit(
    const TStorageConfig& config,
    NCloud::NProto::EStorageMediaKind mediaKind)
{
    using namespace NCloud::NProto;

    ui64 unit = 0;
    switch (mediaKind) {
        case STORAGE_MEDIA_SSD:
            unit = config.GetAllocationUnitSSD() * 1_GB;
            break;

        case STORAGE_MEDIA_SSD_NONREPLICATED:
            unit = config.GetAllocationUnitNonReplicatedSSD() * 1_GB;
            break;

        case STORAGE_MEDIA_SSD_MIRROR2:
            unit = config.GetAllocationUnitMirror2SSD() * 1_GB;
            break;

        case STORAGE_MEDIA_SSD_MIRROR3:
            unit = config.GetAllocationUnitMirror3SSD() * 1_GB;
            break;

        case STORAGE_MEDIA_SSD_LOCAL:
            unit = 4_KB;    // custom pool can have any size
            break;

        default:
            unit = config.GetAllocationUnitHDD() * 1_GB;
            break;
    }

    Y_VERIFY(unit != 0); // TODO: this check should be moved to nbs startup

    return unit;
}

}   // namespace NCloud::NBlockStore::NStorage
