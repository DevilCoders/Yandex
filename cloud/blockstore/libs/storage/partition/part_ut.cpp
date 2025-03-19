#include "part.h"

#include "part_events_private.h"

#include <cloud/blockstore/public/api/protos/volume.pb.h>

#include <cloud/blockstore/libs/common/sglist_test.h>
#include <cloud/blockstore/libs/diagnostics/block_digest.h>
#include <cloud/blockstore/libs/diagnostics/config.h>
#include <cloud/blockstore/libs/diagnostics/critical_events.h>
#include <cloud/blockstore/libs/diagnostics/profile_log.h>
#include <cloud/blockstore/libs/kikimr/helpers.h>
#include <cloud/blockstore/libs/storage/api/partition.h>
#include <cloud/blockstore/libs/storage/api/service.h>
#include <cloud/blockstore/libs/storage/api/stats_service.h>
#include <cloud/blockstore/libs/storage/api/volume.h>
#include <cloud/blockstore/libs/storage/api/volume_proxy.h>
#include <cloud/blockstore/libs/storage/core/block_handler.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/model/channel_data_kind.h>
#include <cloud/blockstore/libs/storage/partition/part.h>
#include <cloud/blockstore/libs/storage/partition_common/events_private.h>
#include <cloud/blockstore/libs/storage/testlib/test_env.h>
#include <cloud/blockstore/libs/storage/testlib/test_runtime.h>

// TODO: invalid reference
#include <cloud/blockstore/libs/storage/service/service_events_private.h>
#include <cloud/blockstore/libs/storage/volume/volume_events_private.h>

#include <cloud/storage/core/libs/api/hive_proxy.h>

#include <ydb/core/base/blobstorage.h>
#include <ydb/core/testlib/basics/storage.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/bitmap.h>
#include <util/generic/size_literals.h>
#include <util/generic/variant.h>

namespace NCloud::NBlockStore::NStorage::NPartition {

using namespace NActors;

using namespace NKikimr;

using namespace NCloud::NStorage;

namespace {

////////////////////////////////////////////////////////////////////////////////

#define BLOCKSTORE_PARTITION_REQUESTS_MON(xxx, ...)                            \
    xxx(RemoteHttpInfo,         __VA_ARGS__)                                   \
// BLOCKSTORE_PARTITION_REQUESTS_MON

////////////////////////////////////////////////////////////////////////////////

constexpr TDuration WaitTimeout = TDuration::Seconds(5);
constexpr ui32 DataChannelOffset = 3;
const TActorId VolumeActorId(0, "VVV");

TString GetBlockContent(char fill = 0, size_t size = DefaultBlockSize)
{
    return TString(size, fill);
}

TString GetBlocksContent(
    char fill = 0,
    ui32 blocksCount = 1,
    size_t blockSize = DefaultBlockSize)
{
    TString result;
    for (ui32 i = 0; i < blocksCount; ++i) {
        result += GetBlockContent(fill, blockSize);
    }
    return result;
}

void CheckRangesArePartition(
    TVector<TBlockRange32> ranges,
    const TBlockRange32& unionRange)
{
    UNIT_ASSERT(ranges.size() > 0);

    Sort(ranges, [](const auto& l, const auto& r) {
            return l.Start < r.Start;
        });

    const auto* prev = &ranges.front();
    UNIT_ASSERT_VALUES_EQUAL(unionRange.Start, prev->Start);

    for (size_t i = 1; i < ranges.size(); ++i) {
        UNIT_ASSERT_VALUES_EQUAL_C(prev->End + 1, ranges[i].Start,
            "during iteration #" << i);
        prev = &ranges[i];
    }

    UNIT_ASSERT_VALUES_EQUAL(unionRange.End, ranges.back().End);
}

NProto::TStorageServiceConfig DefaultConfig(ui32 flushBlobSizeThreshold = 4_KB)
{
    NProto::TStorageServiceConfig config;
    config.SetFlushBlobSizeThreshold(flushBlobSizeThreshold);
    config.SetFreshByteCountThresholdForBackpressure(400_KB);
    config.SetFreshByteCountLimitForBackpressure(1200_KB);
    config.SetFreshByteCountFeatureMaxValue(6);
    config.SetCollectGarbageThreshold(10);

    return config;
}

TDiagnosticsConfigPtr CreateTestDiagnosticsConfig()
{
    return std::make_shared<TDiagnosticsConfig>(NProto::TDiagnosticsConfig());
}

////////////////////////////////////////////////////////////////////////////////

struct TTestPartitionInfo
{
    TString DiskId = "test";
    TString BaseDiskId;
    TString BaseDiskCheckpointId;
    ui64 TabletId = TestTabletId;
    ui64 BaseTabletId = 0;
    NCloud::NProto::EStorageMediaKind MediaKind =
        NCloud::NProto::STORAGE_MEDIA_DEFAULT;
};

////////////////////////////////////////////////////////////////////////////////

class TDummyActor final
    : public TActor<TDummyActor>
{
public:
    TDummyActor()
        : TActor(&TThis::StateWork)
    {
    }

private:
    STFUNC(StateWork)
    {
        Y_UNUSED(ev);
        Y_UNUSED(ctx);
    }
};

////////////////////////////////////////////////////////////////////////////////

void InitTestActorRuntime(
    TTestActorRuntime& runtime,
    const NProto::TStorageServiceConfig& config,
    ui32 blocksCount,
    ui32 channelCount,
    std::unique_ptr<TTabletStorageInfo> tabletInfo,
    TTestPartitionInfo partitionInfo = TTestPartitionInfo(),
    EStorageAccessMode storageAccessMode = EStorageAccessMode::Default)
{
    auto storageConfig = std::make_shared<TStorageConfig>(
        config,
        std::make_shared<TFeaturesConfig>(NProto::TFeaturesConfig())
    );

    NProto::TPartitionConfig partConfig;

    partConfig.SetDiskId(partitionInfo.DiskId);
    partConfig.SetBaseDiskId(partitionInfo.BaseDiskId);
    partConfig.SetBaseDiskCheckpointId(partitionInfo.BaseDiskCheckpointId);
    partConfig.SetStorageMediaKind(partitionInfo.MediaKind);

    partConfig.SetBlockSize(DefaultBlockSize);
    partConfig.SetBlocksCount(blocksCount);

    auto* cps = partConfig.MutableExplicitChannelProfiles();
    cps->Add()->SetDataKind(static_cast<ui32>(EChannelDataKind::System));
    cps->Add()->SetDataKind(static_cast<ui32>(EChannelDataKind::Log));
    cps->Add()->SetDataKind(static_cast<ui32>(EChannelDataKind::Index));

    for (ui32 i = 0; i < channelCount - DataChannelOffset - 1; ++i) {
        cps->Add()->SetDataKind(static_cast<ui32>(EChannelDataKind::Merged));
    }

    cps->Add()->SetDataKind(static_cast<ui32>(EChannelDataKind::Fresh));

    auto diagConfig = CreateTestDiagnosticsConfig();

    auto createFunc =
        [=] (const TActorId& owner, TTabletStorageInfo* info) {
            auto tablet = CreatePartitionTablet(
                owner,
                info,
                storageConfig,
                diagConfig,
                CreateProfileLogStub(),
                CreateBlockDigestGeneratorStub(),
                partConfig,
                storageAccessMode,
                1,  // siblingCount
                VolumeActorId
            );
            return tablet.release();
        };

    auto bootstrapper = CreateTestBootstrapper(runtime, tabletInfo.release(), createFunc);
    runtime.EnableScheduleForActor(bootstrapper);
}

////////////////////////////////////////////////////////////////////////////////

auto InitTestActorRuntime(
    TTestEnv& env,
    TTestActorRuntime& runtime,
    ui32 channelCount,
    ui32 tabletInfoChannelCount,  // usually should be equal to channelCount
    const NProto::TStorageServiceConfig& config = DefaultConfig())
{
    {
        env.CreateSubDomain("nbs");
        auto storageConfig = CreateTestStorageConfig(config);
        env.CreateBlockStoreNode(
            "nbs",
            storageConfig,
            CreateTestDiagnosticsConfig()
        );
    }

    const auto tabletId = NKikimr::MakeTabletID(1, HiveId, 1);
    std::unique_ptr<TTabletStorageInfo> x(new TTabletStorageInfo());

    x->TabletID = tabletId;
    x->TabletType = TTabletTypes::BlockStorePartition;
    auto& channels = x->Channels;
    channels.resize(tabletInfoChannelCount);

    for (ui64 channel = 0; channel < channels.size(); ++channel) {
        channels[channel].Channel = channel;
        channels[channel].Type =
            TBlobStorageGroupType(BootGroupErasure);
        channels[channel].History.resize(1);
        channels[channel].History[0].FromGeneration = 0;
        const auto gidx =
            channel > DataChannelOffset ? channel - DataChannelOffset : 0;
        channels[channel].History[0].GroupID = env.GetGroupIds()[gidx];
    }

    InitTestActorRuntime(runtime, config, 1024, channelCount, std::move(x));

    return tabletId;
}

////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<TTestActorRuntime> PrepareTestActorRuntime(
    const NProto::TStorageServiceConfig& config = DefaultConfig(),
    ui32 blocksCount = 1024,
    TMaybe<ui32> channelsCount = {},
    const TTestPartitionInfo& testPartitionInfo = TTestPartitionInfo(),
    IActorPtr volumeProxy = {},
    EStorageAccessMode storageAccessMode = EStorageAccessMode::Default)
{
    auto runtime = std::make_unique<TTestBasicRuntime>(1);

    if (volumeProxy) {
        runtime->AddLocalService(
            MakeVolumeProxyServiceId(),
            TActorSetupCmd(volumeProxy.release(), TMailboxType::Simple, 0));
    }

    runtime->AddLocalService(
        VolumeActorId,
        TActorSetupCmd(new TDummyActor, TMailboxType::Simple, 0));

    runtime->AddLocalService(
        MakeHiveProxyServiceId(),
        TActorSetupCmd(new TDummyActor, TMailboxType::Simple, 0));

    runtime->AppendToLogSettings(
        TBlockStoreComponents::START,
        TBlockStoreComponents::END,
        GetComponentName);

    // for (ui32 i = TBlockStoreComponents::START; i < TBlockStoreComponents::END; ++i) {
    //    runtime->SetLogPriority(i, NLog::PRI_DEBUG);
    // }
    // runtime->SetLogPriority(NLog::InvalidComponent, NLog::PRI_DEBUG);
    runtime->SetLogPriority(NKikimrServices::BS_NODE, NLog::PRI_ERROR);

    SetupTabletServices(*runtime);

    std::unique_ptr<TTabletStorageInfo> tabletInfo(CreateTestTabletInfo(
        testPartitionInfo.TabletId,
        TTabletTypes::BlockStorePartition));

    if (channelsCount) {
        auto& channels = tabletInfo->Channels;
        channels.resize(*channelsCount);

        for (ui64 i = 0; i < channels.size(); ++i) {
            auto& channel = channels[i];
            channel.History.resize(1);
        }
    }

    InitTestActorRuntime(
        *runtime,
        config,
        blocksCount,
        channelsCount ? *channelsCount : tabletInfo->Channels.size(),
        std::move(tabletInfo),
        testPartitionInfo,
        storageAccessMode
    );

    return runtime;
}

////////////////////////////////////////////////////////////////////////////////

template <typename T>
void AssertEqual(const TVector<T>& l, const TVector<T>& r)
{
    UNIT_ASSERT_VALUES_EQUAL(l.size(), r.size());
    for (size_t i = 0; i < l.size(); ++i) {
        UNIT_ASSERT_VALUES_EQUAL(l[i], r[i]);
    }
}

////////////////////////////////////////////////////////////////////////////////

TString GetBlockContent(
    const std::unique_ptr<TEvService::TEvReadBlocksResponse>& response)
{
    if (response->Record.GetBlocks().BuffersSize() == 1) {
        return response->Record.GetBlocks().GetBuffers(0);
    }
    return {};
}

TString GetBlocksContent(
    const std::unique_ptr<TEvService::TEvReadBlocksResponse>& response)
{
    const auto& blocks = response->Record.GetBlocks();

    {
        bool empty = true;
        for (size_t i = 0; i < blocks.BuffersSize(); ++i) {
            if (blocks.GetBuffers(i)) {
                empty = false;
            }
        }

        if (empty) {
            return TString();
        }
    }

    TString result;

    for (size_t i = 0; i < blocks.BuffersSize(); ++i) {
        const auto& block = blocks.GetBuffers(i);
        if (!block) {
            result += GetBlockContent(char(0));
            continue;
        }
        result += block;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////

TDynBitMap GetBitMap(const TString& s)
{
    TDynBitMap mask;

    if (s) {
        mask.Reserve(s.size() * 8);
        Y_VERIFY(mask.GetChunkCount() * sizeof(TDynBitMap::TChunk) == s.size());
        auto* dst = const_cast<TDynBitMap::TChunk*>(mask.GetChunks());
        memcpy(dst, s.data(), s.size());
    }

    return mask;
}

TDynBitMap GetUnencryptedBlockMask(
    const std::unique_ptr<TEvService::TEvReadBlocksResponse>& response)
{
    return GetBitMap(response->Record.GetUnencryptedBlockMask());
}

TDynBitMap GetUnencryptedBlockMask(
    const std::unique_ptr<TEvService::TEvReadBlocksLocalResponse>& response)
{
    return GetBitMap(response->Record.GetUnencryptedBlockMask());
}

TDynBitMap CreateBitmap(size_t size)
{
    TDynBitMap bitmap;
    bitmap.Reserve(size);
    bitmap.Set(0, size);
    return bitmap;
}

void MarkZeroedBlocks(TDynBitMap& bitmap, const TBlockRange32& range)
{
    if (range.End >= bitmap.Size()) {
        bitmap.Reserve(range.End);
    }
    bitmap.Set(range.Start, range.End + 1);
}

void MarkWrittenBlocks(TDynBitMap& bitmap, const TBlockRange32& range)
{
    if (range.End >= bitmap.Size()) {
        bitmap.Reserve(range.End);
    }
    bitmap.Reset(range.Start, range.End + 1);
}

////////////////////////////////////////////////////////////////////////////////

class TPartitionClient
{
private:
    TTestActorRuntime& Runtime;
    ui32 NodeIdx = 0;
    ui64 TabletId;

    const TActorId Sender;
    TActorId PipeClient;

public:
    TPartitionClient(
            TTestActorRuntime& runtime,
            ui32 nodeIdx = 0,
            ui64 tabletId = TestTabletId)
        : Runtime(runtime)
        , NodeIdx(nodeIdx)
        , TabletId(tabletId)
        , Sender(runtime.AllocateEdgeActor(NodeIdx))
    {
        PipeClient = Runtime.ConnectToPipe(
            TabletId,
            Sender,
            NodeIdx,
            NKikimr::GetPipeConfigWithRetries());
    }

    void RebootTablet()
    {
        TVector<ui64> tablets = { TabletId };
        auto guard = CreateTabletScheduledEventsGuard(
            tablets,
            Runtime,
            Sender);

        NKikimr::RebootTablet(Runtime, TabletId, Sender);

        // sooner or later after reset pipe will reconnect
        // but we do not want to wait
        PipeClient = Runtime.ConnectToPipe(
            TabletId,
            Sender,
            NodeIdx,
            NKikimr::GetPipeConfigWithRetries());
    }

    void KillTablet()
    {
        SendToPipe(std::make_unique<TEvents::TEvPoisonPill>());
        Runtime.DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));
    }

    template <typename TRequest>
    void SendToPipe(std::unique_ptr<TRequest> request, ui64 cookie = 0)
    {
        Runtime.SendToPipe(
            PipeClient,
            Sender,
            request.release(),
            NodeIdx,
            cookie);
    }

    template <typename TResponse>
    std::unique_ptr<TResponse> RecvResponse()
    {
        TAutoPtr<IEventHandle> handle;
        Runtime.GrabEdgeEventRethrow<TResponse>(handle, WaitTimeout);

        UNIT_ASSERT(handle);
        return std::unique_ptr<TResponse>(handle->Release<TResponse>().Release());
    }

    std::unique_ptr<TEvPartition::TEvStatPartitionRequest> CreateStatPartitionRequest()
    {
        return std::make_unique<TEvPartition::TEvStatPartitionRequest>();
    }

    std::unique_ptr<TEvService::TEvWriteBlocksRequest> CreateWriteBlocksRequest(
        ui32 blockIndex,
        TString blockContent)
    {
        auto request = std::make_unique<TEvService::TEvWriteBlocksRequest>();
        request->Record.SetStartIndex(blockIndex);
        *request->Record.MutableBlocks()->MutableBuffers()->Add() = blockContent;
        return request;
    }

    std::unique_ptr<TEvService::TEvWriteBlocksRequest> CreateWriteBlocksRequest(
        const TBlockRange32& writeRange,
        char fill = 0)
    {
        auto blockContent = GetBlockContent(fill);

        auto request = std::make_unique<TEvService::TEvWriteBlocksRequest>();
        request->Record.SetStartIndex(writeRange.Start);

        auto& buffers = *request->Record.MutableBlocks()->MutableBuffers();
        for (ui32 i = 0; i < writeRange.Size(); ++i) {
            *buffers.Add() = blockContent;
        }

        return request;
    }

    std::unique_ptr<TEvService::TEvWriteBlocksLocalRequest> CreateWriteBlocksLocalRequest(
        const TBlockRange32& writeRange,
        TStringBuf blockContent)
    {
        TSgList sglist;
        sglist.resize(writeRange.Size(), {blockContent.Data(), blockContent.Size()});

        auto request = std::make_unique<TEvService::TEvWriteBlocksLocalRequest>();
        request->Record.SetStartIndex(writeRange.Start);
        request->Record.Sglist = TGuardedSgList(std::move(sglist));
        request->Record.BlocksCount = writeRange.Size();
        request->Record.BlockSize = blockContent.Size() / writeRange.Size();
        return request;
    }

    std::unique_ptr<TEvService::TEvWriteBlocksRequest> CreateWriteBlocksRequest(
        ui32 blockIndex,
        char fill = 0)
    {
        return CreateWriteBlocksRequest(blockIndex, GetBlockContent(fill));
    }

    std::unique_ptr<TEvService::TEvWriteBlocksLocalRequest> CreateWriteBlocksLocalRequest(
        ui32 blockIndex,
        TStringBuf blockContent)
    {
        return CreateWriteBlocksLocalRequest(
            {blockIndex, blockIndex},
            std::move(blockContent));
    }

    std::unique_ptr<TEvService::TEvZeroBlocksRequest> CreateZeroBlocksRequest(
        ui32 blockIndex)
    {
        auto request = std::make_unique<TEvService::TEvZeroBlocksRequest>();
        request->Record.SetStartIndex(blockIndex);
        request->Record.SetBlocksCount(1);
        return request;
    }

    std::unique_ptr<TEvService::TEvZeroBlocksRequest> CreateZeroBlocksRequest(
        const TBlockRange32& writeRange)
    {
        auto request = std::make_unique<TEvService::TEvZeroBlocksRequest>();
        request->Record.SetStartIndex(writeRange.Start);
        request->Record.SetBlocksCount(writeRange.Size());
        return request;
    }

    std::unique_ptr<TEvService::TEvReadBlocksRequest> CreateReadBlocksRequest(
        const TBlockRange32& range,
        const TString& checkpointId = {})
    {
        auto request = std::make_unique<TEvService::TEvReadBlocksRequest>();
        request->Record.SetStartIndex(range.Start);
        request->Record.SetBlocksCount(range.Size());
        request->Record.SetCheckpointId(checkpointId);
        return request;
    }

    std::unique_ptr<TEvService::TEvReadBlocksRequest> CreateReadBlocksRequest(
        ui32 blockIndex,
        const TString& checkpointId = {})
    {
        return CreateReadBlocksRequest(TBlockRange32::WithLength(blockIndex, 1), checkpointId);
    }

    std::unique_ptr<TEvService::TEvReadBlocksLocalRequest> CreateReadBlocksLocalRequest(
        ui32 blockIndex,
        TStringBuf buffer,
        const TString& checkpointId = {})
    {
        return CreateReadBlocksLocalRequest(blockIndex, TGuardedSgList{{TBlockDataRef(buffer.Data(), buffer.Size())}}, checkpointId);
    }

    std::unique_ptr<TEvService::TEvReadBlocksLocalRequest> CreateReadBlocksLocalRequest(
        ui32 blockIndex,
        const TGuardedSgList& sglist,
        const TString& checkpointId = {})
    {
        return CreateReadBlocksLocalRequest({blockIndex, blockIndex}, sglist, checkpointId);
    }

    std::unique_ptr<TEvService::TEvReadBlocksLocalRequest> CreateReadBlocksLocalRequest(
        const TBlockRange32& readRange,
        const TSgList& sglist,
        const TString& checkpointId = {})
    {
        return CreateReadBlocksLocalRequest(readRange, TGuardedSgList(sglist), checkpointId);
    }

    std::unique_ptr<TEvService::TEvReadBlocksLocalRequest> CreateReadBlocksLocalRequest(
        const TBlockRange32& readRange,
        const TGuardedSgList& sglist,
        const TString& checkpointId = {})
    {
        auto request = std::make_unique<TEvService::TEvReadBlocksLocalRequest>();
        request->Record.SetCheckpointId(checkpointId);
        request->Record.SetStartIndex(readRange.Start);
        request->Record.SetBlocksCount(readRange.Size());

        request->Record.Sglist = sglist;
        request->Record.BlockSize = DefaultBlockSize;
        return request;
    }

    std::unique_ptr<TEvService::TEvCreateCheckpointRequest> CreateCreateCheckpointRequest(
        const TString& checkpointId)
    {
        auto request = std::make_unique<TEvService::TEvCreateCheckpointRequest>();
        request->Record.SetCheckpointId(checkpointId);
        return request;
    }

    std::unique_ptr<TEvService::TEvCreateCheckpointRequest> CreateCreateCheckpointRequest(
        const TString& checkpointId,
        const TString& idempotenceId)
    {
        auto request = std::make_unique<TEvService::TEvCreateCheckpointRequest>();
        request->Record.SetCheckpointId(checkpointId);
        request->Record.MutableHeaders()->SetIdempotenceId(idempotenceId);
        return request;
    }

    std::unique_ptr<TEvService::TEvDeleteCheckpointRequest> CreateDeleteCheckpointRequest(
        const TString& checkpointId)
    {
        auto request = std::make_unique<TEvService::TEvDeleteCheckpointRequest>();
        request->Record.SetCheckpointId(checkpointId);
        return request;
    }

    std::unique_ptr<TEvService::TEvDeleteCheckpointRequest> CreateDeleteCheckpointRequest(
        const TString& checkpointId,
        const TString& idempotenceId)
    {
        auto request = std::make_unique<TEvService::TEvDeleteCheckpointRequest>();
        request->Record.SetCheckpointId(checkpointId);
        request->Record.MutableHeaders()->SetIdempotenceId(idempotenceId);
        return request;
    }

    std::unique_ptr<TEvVolume::TEvDeleteCheckpointDataRequest> CreateDeleteCheckpointDataRequest(
        const TString& checkpointId)
    {
        auto request = std::make_unique<TEvVolume::TEvDeleteCheckpointDataRequest>();
        request->Record.SetCheckpointId(checkpointId);
        return request;
    }

    std::unique_ptr<TEvService::TEvGetChangedBlocksRequest> CreateGetChangedBlocksRequest(
        const TBlockRange32& range,
        const TString& lowCheckpointId,
        const TString& highCheckpointId)
    {
        auto request = std::make_unique<TEvService::TEvGetChangedBlocksRequest>();
        request->Record.SetStartIndex(range.Start);
        request->Record.SetBlocksCount(range.Size());
        request->Record.SetLowCheckpointId(lowCheckpointId);
        request->Record.SetHighCheckpointId(highCheckpointId);
        return request;
    }

    std::unique_ptr<TEvPartition::TEvWaitReadyRequest> CreateWaitReadyRequest()
    {
        return std::make_unique<TEvPartition::TEvWaitReadyRequest>();
    }

    std::unique_ptr<TEvPartitionPrivate::TEvFlushRequest> CreateFlushRequest()
    {
        return std::make_unique<TEvPartitionPrivate::TEvFlushRequest>();
    }

    std::unique_ptr<TEvPartitionCommonPrivate::TEvTrimFreshLogRequest> CreateTrimFreshLogRequest()
    {
        return std::make_unique<TEvPartitionCommonPrivate::TEvTrimFreshLogRequest>();
    }

    std::unique_ptr<TEvPartitionPrivate::TEvCompactionRequest> CreateCompactionRequest()
    {
        return std::make_unique<TEvPartitionPrivate::TEvCompactionRequest>();
    }

    std::unique_ptr<TEvPartitionPrivate::TEvCompactionRequest> CreateCompactionRequest(ui32 blockIndex, bool forceFullCompaction = false)
    {
        return std::make_unique<TEvPartitionPrivate::TEvCompactionRequest>(blockIndex, forceFullCompaction);
    }

    std::unique_ptr<TEvPartitionPrivate::TEvMetadataRebuildBlockCountRequest> CreateMetadataRebuildBlockCountRequest(
        TPartialBlobId blobId,
        ui32 count,
        TPartialBlobId lastBlobId)
    {
        return std::make_unique<TEvPartitionPrivate::TEvMetadataRebuildBlockCountRequest>(
            blobId,
            count,
            lastBlobId,
            TBlockCountRebuildState());
    }

    std::unique_ptr<TEvPartitionPrivate::TEvCleanupRequest> CreateCleanupRequest()
    {
        return std::make_unique<TEvPartitionPrivate::TEvCleanupRequest>();
    }

    std::unique_ptr<TEvTablet::TEvGetCounters> CreateGetCountersRequest()
    {
        auto request = std::make_unique<TEvTablet::TEvGetCounters>();
        return request;
    }

    void SendGetCountersRequest()
    {
        auto request = CreateGetCountersRequest();
        SendToPipe(std::move(request));
    }

    std::unique_ptr<TEvTablet::TEvGetCountersResponse> RecvGetCountersResponse()
    {
        return RecvResponse<TEvTablet::TEvGetCountersResponse>();
    }

    std::unique_ptr<TEvTablet::TEvGetCountersResponse> GetCounters()
    {
        auto request = CreateGetCountersRequest();
        SendToPipe(std::move(request));

        auto response = RecvResponse<TEvTablet::TEvGetCountersResponse>();
        return response;
    }

    std::unique_ptr<TEvPartitionPrivate::TEvCollectGarbageRequest> CreateCollectGarbageRequest()
    {
        return std::make_unique<TEvPartitionPrivate::TEvCollectGarbageRequest>();
    }

    std::unique_ptr<TEvVolume::TEvDescribeBlocksRequest> CreateDescribeBlocksRequest(
        ui32 startIndex,
        ui32 blocksCount,
        const TString& checkpointId = "")
    {
        auto request = std::make_unique<TEvVolume::TEvDescribeBlocksRequest>();
        request->Record.SetStartIndex(startIndex);
        request->Record.SetBlocksCount(blocksCount);
        request->Record.SetCheckpointId(checkpointId);
        return request;
    }

    std::unique_ptr<TEvVolume::TEvDescribeBlocksRequest> CreateDescribeBlocksRequest(
        const TBlockRange32& range, const TString& checkpointId = "")
    {
        return CreateDescribeBlocksRequest(range.Start, range.Size(), checkpointId);
    }

    std::unique_ptr<TEvVolume::TEvGetUsedBlocksRequest> CreateGetUsedBlocksRequest()
    {
        return std::make_unique<TEvVolume::TEvGetUsedBlocksRequest>();
    }

    std::unique_ptr<TEvPartitionPrivate::TEvReadBlobRequest> CreateReadBlobRequest(
        const NKikimr::TLogoBlobID& blobId,
        const ui32 bSGroupId,
        const TVector<ui16>& blobOffsets,
        TSgList sglist)
    {
        auto request =
            std::make_unique<TEvPartitionPrivate::TEvReadBlobRequest>(
                blobId,
                MakeBlobStorageProxyID(bSGroupId),
                blobOffsets,
                TGuardedSgList(std::move(sglist)),
                bSGroupId);
        return request;
    }

    std::unique_ptr<TEvVolume::TEvCompactRangeRequest> CreateCompactRangeRequest(
        ui32 blockIndex,
        ui32 blocksCount)
    {
        auto request = std::make_unique<TEvVolume::TEvCompactRangeRequest>();
        request->Record.SetStartIndex(blockIndex);
        request->Record.SetBlocksCount(blocksCount);
        return request;
    }

    std::unique_ptr<TEvVolume::TEvGetCompactionStatusRequest> CreateGetCompactionStatusRequest(
        const TString& operationId)
    {
        auto request = std::make_unique<TEvVolume::TEvGetCompactionStatusRequest>();
        request->Record.SetOperationId(operationId);
        return request;
    }

    std::unique_ptr<TEvPartition::TEvDrainRequest> CreateDrainRequest()
    {
        return std::make_unique<TEvPartition::TEvDrainRequest>();
    }

    std::unique_ptr<NMon::TEvRemoteHttpInfo> CreateRemoteHttpInfo(
        const TString& params,
        HTTP_METHOD method)
    {
        return std::make_unique<NMon::TEvRemoteHttpInfo>(params, method);
    }

    std::unique_ptr<NMon::TEvRemoteHttpInfo> CreateRemoteHttpInfo(
        const TString& params)
    {
        return std::make_unique<NMon::TEvRemoteHttpInfo>(params);
    }

    void SendRemoteHttpInfo(
        const TString& params,
        HTTP_METHOD method)
    {
        auto request = CreateRemoteHttpInfo(params, method);
        SendToPipe(std::move(request));
    }

    void SendRemoteHttpInfo(
        const TString& params)
    {
        auto request = CreateRemoteHttpInfo(params);
        SendToPipe(std::move(request));
    }

    std::unique_ptr<NMon::TEvRemoteHttpInfoRes> RecvCreateRemoteHttpInfoRes()
    {
        return RecvResponse<NMon::TEvRemoteHttpInfoRes>();
    }

    std::unique_ptr<NMon::TEvRemoteHttpInfoRes> RemoteHttpInfo(
        const TString& params,
        HTTP_METHOD method)
    {
        auto request = CreateRemoteHttpInfo(params, method);
        SendToPipe(std::move(request));

        auto response = RecvResponse<NMon::TEvRemoteHttpInfoRes>();
        return response;
    }

    std::unique_ptr<NMon::TEvRemoteHttpInfoRes> RemoteHttpInfo(
        const TString& params)
    {
        return RemoteHttpInfo(params, HTTP_METHOD::HTTP_METHOD_GET);
    }

    std::unique_ptr<TEvVolume::TEvRebuildMetadataRequest> CreateRebuildMetadataRequest(
        NProto::ERebuildMetadataType type,
        ui32 batchSize)
    {
        auto request = std::make_unique<TEvVolume::TEvRebuildMetadataRequest>();
        request->Record.SetMetadataType(type);
        request->Record.SetBatchSize(batchSize);
        return request;
    }

    std::unique_ptr<TEvVolume::TEvGetRebuildMetadataStatusRequest> CreateGetRebuildMetadataStatusRequest()
    {
        auto request = std::make_unique<TEvVolume::TEvGetRebuildMetadataStatusRequest>();
        return request;
    }

#define BLOCKSTORE_DECLARE_METHOD(name, ns)                                    \
    template <typename... Args>                                                \
    void Send##name##Request(Args&&... args)                                   \
    {                                                                          \
        auto request = Create##name##Request(std::forward<Args>(args)...);     \
        SendToPipe(std::move(request));                                        \
    }                                                                          \
                                                                               \
    std::unique_ptr<ns::TEv##name##Response> Recv##name##Response()            \
    {                                                                          \
        return RecvResponse<ns::TEv##name##Response>();                        \
    }                                                                          \
                                                                               \
    template <typename... Args>                                                \
    std::unique_ptr<ns::TEv##name##Response> name(Args&&... args)              \
    {                                                                          \
        auto request = Create##name##Request(std::forward<Args>(args)...);     \
        SendToPipe(std::move(request));                                        \
                                                                               \
        auto response = RecvResponse<ns::TEv##name##Response>();               \
        UNIT_ASSERT_C(                                                         \
            SUCCEEDED(response->GetStatus()),                                  \
            response->GetErrorReason());                                       \
        return response;                                                       \
    }                                                                          \
// BLOCKSTORE_DECLARE_METHOD

    BLOCKSTORE_PARTITION_REQUESTS(BLOCKSTORE_DECLARE_METHOD, TEvPartition)
    BLOCKSTORE_PARTITION_REQUESTS_PRIVATE(BLOCKSTORE_DECLARE_METHOD, TEvPartitionPrivate)
    BLOCKSTORE_PARTITION_COMMON_REQUESTS_PRIVATE(BLOCKSTORE_DECLARE_METHOD, TEvPartitionCommonPrivate)
    BLOCKSTORE_PARTITION_REQUESTS_FWD_SERVICE(BLOCKSTORE_DECLARE_METHOD, TEvService)
    BLOCKSTORE_PARTITION_REQUESTS_FWD_VOLUME(BLOCKSTORE_DECLARE_METHOD, TEvVolume)

#undef BLOCKSTORE_DECLARE_METHOD
};

TTestActorRuntime::TEventObserver StorageStateChanger(
    ui32  flag,
    TMaybe<ui32> groupIdFilter = {})
{
    return [=] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
        switch (event->GetTypeRewrite()) {
            case TEvBlobStorage::EvPutResult: {
                auto* msg = event->Get<TEvBlobStorage::TEvPutResult>();
                if (!groupIdFilter.Defined() || *groupIdFilter == msg->GroupId) {
                    const_cast<TStorageStatusFlags&>(msg->StatusFlags).Merge(
                        ui32(NKikimrBlobStorage::StatusIsValid) | ui32(flag)
                    );
                    break;
                }
            }
        }

        return TTestActorRuntime::DefaultObserverFunc(runtime, event);
    };
}

TTestActorRuntime::TEventObserver PartitionBatchWriteCollector(ui32 eventCount)
{
    bool dropProcessWriteQueue = true;
    ui32 cnt = 0;
    bool batchSeen = false;
    NActors::TActorId partActorId;
    return [=] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) mutable {
        switch (event->GetTypeRewrite()) {
            case TEvPartitionPrivate::EvProcessWriteQueue: {
                batchSeen = true;
                if (dropProcessWriteQueue) {
                    partActorId = event->Sender;
                    return TTestActorRuntime::EEventAction::DROP;
                }
                break;
            }
            case TEvService::EvWriteBlocksRequest: {
                if (++cnt == eventCount && batchSeen) {
                    dropProcessWriteQueue = false;
                    auto req =
                        std::make_unique<TEvPartitionPrivate::TEvProcessWriteQueue>();
                    runtime.Send(
                        new IEventHandle(
                            partActorId,
                            event->Sender,
                            req.release(),
                            0, // flags
                            0),
                        0);
                }
                break;
            }
        }
        return TTestActorRuntime::DefaultObserverFunc(runtime, event);
    };
}

////////////////////////////////////////////////////////////////////////////////

struct TEmpty {};

struct TBlob
{
    TBlob(
        ui32 number,
        ui8 offset,
        ui8 blocksCount = 1,
        ui32 channel = 0,
        ui32 generation = 0
    )
        : Number(number)
        , Offset(offset)
        , BlocksCount(blocksCount)
        , Channel(channel)
        , Generation(generation)
    {}

    ui32 Number;
    ui8 Offset;
    ui8 BlocksCount;
    ui32 Channel;
    ui32 Generation;
};

struct TFresh
{
    TFresh(ui8 value)
        : Value(value)
    {}

    ui8 Value;
};

using TBlockDescription = std::variant<TEmpty, TBlob, TFresh>;

using TPartitionContent = TVector<TBlockDescription>;

////////////////////////////////////////////////////////////////////////////////

struct TPartitionWithRuntime
{
    std::unique_ptr<TTestActorRuntime> Runtime;
    std::unique_ptr<TPartitionClient> Partition;
};

////////////////////////////////////////////////////////////////////////////////

class TTestVolumeProxyActor final
    : public TActorBootstrapped<TTestVolumeProxyActor>
{
private:
    ui64 BaseTabletId;
    TString BaseDiskId;
    TString BaseDiskCheckpointId;
    TPartitionContent BasePartitionContent;
    ui32 BlocksCount;
    ui32 BaseBlockSize;

public:
    TTestVolumeProxyActor(
        ui64 baseTabletId,
        const TString& baseDiskId,
        const TString& baseDiskCheckpointId,
        const TPartitionContent& basePartitionContent,
        ui32 blocksCount,
        ui32 baseBlockSize = DefaultBlockSize);

    void Bootstrap(const TActorContext& ctx);

private:
    STFUNC(StateWork);

    void HandleDescribeBlocksRequest(
        const TEvVolume::TEvDescribeBlocksRequest::TPtr& ev,
        const TActorContext& ctx);

    void HandleGetUsedBlocksRequest(
        const TEvVolume::TEvGetUsedBlocksRequest::TPtr& ev,
        const TActorContext& ctx);

    void HandleGetChangedBlocksRequest(
        const TEvService::TEvGetChangedBlocksRequest::TPtr& ev,
        const TActorContext& ctx);
};

////////////////////////////////////////////////////////////////////////////////

TTestVolumeProxyActor::TTestVolumeProxyActor(
        ui64 baseTabletId,
        const TString& baseDiskId,
        const TString& baseDiskCheckpointId,
        const TPartitionContent& basePartitionContent,
        ui32 blocksCount,
        ui32 baseBlockSize)
    : BaseTabletId(baseTabletId)
    , BaseDiskId(baseDiskId)
    , BaseDiskCheckpointId(baseDiskCheckpointId)
    , BasePartitionContent(std::move(basePartitionContent))
    , BlocksCount(blocksCount)
    , BaseBlockSize(baseBlockSize)
{
    ActivityType = TBlockStoreActivities::VOLUME_PROXY;
}

void TTestVolumeProxyActor::Bootstrap(const TActorContext& ctx)
{
    Y_UNUSED(ctx);

    Become(&TThis::StateWork);
}

void TTestVolumeProxyActor::HandleDescribeBlocksRequest(
    const TEvVolume::TEvDescribeBlocksRequest::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();
    const auto& record = msg->Record;
    const auto& diskId = record.GetDiskId();
    const auto& checkpoint = record.GetCheckpointId();

    UNIT_ASSERT_VALUES_EQUAL(BaseDiskId, diskId);
    UNIT_ASSERT_VALUES_EQUAL(BaseDiskCheckpointId, checkpoint);

    auto response = std::make_unique<TEvVolume::TEvDescribeBlocksResponse>();
    auto blockIndex = 0;

    for (const auto& descr: BasePartitionContent) {
        if (std::holds_alternative<TBlob>(descr)) {
            const auto& blob = std::get<TBlob>(descr);
            auto& blobPiece = *response->Record.AddBlobPieces();
            NKikimr::TLogoBlobID blobId(
                BaseTabletId,
                blob.Generation,
                blob.Number,
                blob.Channel,
                BaseBlockSize * 0x100,
                0);
            LogoBlobIDFromLogoBlobID(
                blobId,
                blobPiece.MutableBlobId());

            auto* range = blobPiece.AddRanges();
            range->SetBlobOffset(blob.Offset);
            range->SetBlockIndex(blockIndex);
            range->SetBlocksCount(blob.BlocksCount);
            blockIndex += blob.BlocksCount;
        } else if (std::holds_alternative<TFresh>(descr)) {
            const auto& fresh = std::get<TFresh>(descr);
            auto& freshBlockRange = *response->Record.AddFreshBlockRanges();
            freshBlockRange.SetStartIndex(blockIndex);
            freshBlockRange.SetBlocksCount(1);
            freshBlockRange.SetBlocksContent(
                TString(BaseBlockSize, char(fresh.Value)));
            ++blockIndex;
        } else {
            Y_VERIFY(std::holds_alternative<TEmpty>(descr));
            ++blockIndex;
        }
    }

    ctx.Send(ev->Sender, response.release());
}

void TTestVolumeProxyActor::HandleGetUsedBlocksRequest(
    const TEvVolume::TEvGetUsedBlocksRequest::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();
    const auto& record = msg->Record;
    const auto& diskId = record.GetDiskId();

    UNIT_ASSERT_VALUES_EQUAL(BaseDiskId, diskId);

    auto response = std::make_unique<TEvVolume::TEvGetUsedBlocksResponse>();
    ui64 blockIndex = 0;

    TCompressedBitmap bitmap(BlocksCount);

    for (const auto& descr: BasePartitionContent) {
        if (std::holds_alternative<TBlob>(descr)) {
            const auto& blob = std::get<TBlob>(descr);
            bitmap.Set(blockIndex, blockIndex + blob.BlocksCount);
            blockIndex += blob.BlocksCount;
        } else if (std::holds_alternative<TFresh>(descr)) {
            bitmap.Set(blockIndex, blockIndex + 1);
            ++blockIndex;
        } else {
            Y_VERIFY(std::holds_alternative<TEmpty>(descr));
            ++blockIndex;
        }
    }

    auto serializer = bitmap.RangeSerializer(0, bitmap.Capacity());
    TCompressedBitmap::TSerializedChunk chunk;
    while (serializer.Next(&chunk)) {
        if (!TCompressedBitmap::IsZeroChunk(chunk)) {
            auto* usedBlock = response->Record.AddUsedBlocks();
            usedBlock->SetChunkIdx(chunk.ChunkIdx);
            usedBlock->SetData(chunk.Data.data(), chunk.Data.size());
        }
    }

    ctx.Send(ev->Sender, response.release());
}

void TTestVolumeProxyActor::HandleGetChangedBlocksRequest(
    const TEvService::TEvGetChangedBlocksRequest::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();
    const auto& record = msg->Record;

    UNIT_ASSERT_VALUES_EQUAL(BaseDiskId, record.GetDiskId());
    UNIT_ASSERT_VALUES_EQUAL(BaseDiskCheckpointId, record.GetHighCheckpointId());
    UNIT_ASSERT_VALUES_EQUAL("", record.GetLowCheckpointId());

    auto response = std::make_unique<TEvService::TEvGetChangedBlocksResponse>();
    ui64 blockIndex = 0;

    TVector<ui8> changedBlocks((record.GetBlocksCount() + 7) / 8);

    auto fillBlock = [&](ui64 block) {
        if (block < record.GetStartIndex() || block >= record.GetStartIndex() + record.GetBlocksCount()) {
            return;
        }

        ui64 bit = block - record.GetStartIndex();
        changedBlocks[bit / 8] |= 1 << (bit % 8);
    };

    for (const auto& descr: BasePartitionContent) {
        if (std::holds_alternative<TBlob>(descr)) {
            const auto& blob = std::get<TBlob>(descr);
            for (ui64 block = blockIndex; block < blockIndex + blob.BlocksCount; block++) {
                fillBlock(block);
            }
            blockIndex += blob.BlocksCount;
        } else if (std::holds_alternative<TFresh>(descr)) {
            fillBlock(blockIndex);
            ++blockIndex;
        } else {
            Y_VERIFY(std::holds_alternative<TEmpty>(descr));
            ++blockIndex;
        }
    }

    for (const auto& b: changedBlocks) {
        response->Record.MutableMask()->push_back(b);
    }

    ctx.Send(ev->Sender, response.release());
}

STFUNC(TTestVolumeProxyActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvVolume::TEvDescribeBlocksRequest, HandleDescribeBlocksRequest);
        HFunc(TEvVolume::TEvGetUsedBlocksRequest, HandleGetUsedBlocksRequest);
        HFunc(TEvService::TEvGetChangedBlocksRequest, HandleGetChangedBlocksRequest);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::VOLUME_PROXY);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////

bool EventHandler(
    ui64 tabletId,
    const TEvBlobStorage::TEvGet::TPtr& ev,
    TTestActorRuntimeBase& runtime,
    ui32 blockSize = DefaultBlockSize)
{
    bool result = false;

    auto* msg = ev->Get();

    auto response =
        std::make_unique<TEvBlobStorage::TEvGetResult>(
            NKikimrProto::OK, msg->QuerySize, 0 /* groupId */);

    for (ui32 i = 0; i < msg->QuerySize; ++i) {
        const auto& q = msg->Queries[i];
        const auto& blobId = q.Id;

        UNIT_ASSERT_C(
            !result || blobId.TabletID() == tabletId,
            "All blobs in one TEvGet request should belong to one partition;"
            " tabletId: " << tabletId <<
            " blobId: " << blobId);

        if (blobId.TabletID() == tabletId) {
            result = true;

            auto& r = response->Responses[i];

            r.Id = blobId;
            r.Status = NKikimrProto::OK;

            UNIT_ASSERT(q.Size % blockSize == 0);
            UNIT_ASSERT(q.Shift % blockSize == 0);

            auto blobOffset = q.Shift / blockSize;

            const auto blobEnd = blobOffset + q.Size / blockSize;
            for (; blobOffset < blobEnd; ++blobOffset) {
                UNIT_ASSERT_C(
                    blobOffset <= 0xff,
                    "Blob offset should fit in one byte");

                // Debugging is easier when block content is equal to blob offset.
                r.Buffer += TString(blockSize, char(blobOffset));
            }
        }
    }

    if (result) {
        runtime.Schedule(
            new IEventHandle(
                ev->Sender,
                ev->Recipient,
                response.release(),
                0,
                ev->Cookie),
            TDuration());
    }
    return result;
}

TPartitionWithRuntime SetupOverlayPartition(
    ui64 overlayTabletId,
    ui64 baseTabletId,
    const TPartitionContent& basePartitionContent,
    TMaybe<ui32> channelsCount = {},
    ui32 blockSize = DefaultBlockSize,
    ui32 blocksCount = 1024,
    const NProto::TStorageServiceConfig& config = DefaultConfig())
{
    TPartitionWithRuntime result;

    result.Runtime = PrepareTestActorRuntime(
        config,
        blocksCount,
        channelsCount,
        {
            "overlay-disk",
            "base-disk",
            "checkpoint",
            overlayTabletId,
            baseTabletId,
            NCloud::NProto::STORAGE_MEDIA_DEFAULT
        },
        std::make_unique<TTestVolumeProxyActor>(
            baseTabletId,
            "base-disk",
            "checkpoint",
            basePartitionContent,
            blocksCount,
            blockSize));

    result.Partition = std::make_unique<TPartitionClient>(*result.Runtime);
    result.Partition->WaitReady();

    result.Runtime->SetEventFilter([baseTabletId, blockSize] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& ev) {
        bool handled = false;

        const auto wrapped =
            [&] (const auto& ev) {
                handled = EventHandler(baseTabletId, ev, runtime, blockSize);
            };

        switch (ev->GetTypeRewrite()) {
            hFunc(TEvBlobStorage::TEvGet, wrapped);
        }
        return handled;
    });

    return result;
}

TString GetBlocksContent(
    const TPartitionContent& content,
    size_t blockSize = DefaultBlockSize)
{
    TString result;

    for (const auto& descr: content) {
        if (std::holds_alternative<TEmpty>(descr)) {
            result += TString(blockSize, char(0));
        } else if (std::holds_alternative<TBlob>(descr)) {
            const auto& blob = std::get<TBlob>(descr);
            for (auto i = 0; i < blob.BlocksCount; ++i) {
                const auto blobOffset = blob.Offset + i;
                // Debugging is easier when block content is equal to blob offset.
                result += TString(blockSize, char(blobOffset));
            }
        } else if (std::holds_alternative<TFresh>(descr)) {
            const auto& fresh = std::get<TFresh>(descr);
            result += TString(blockSize, char(fresh.Value));
        } else {
            Y_VERIFY(false);
        }
    }

    return result;
}

TString BuildRemoteHttpQuery(ui64 tabletId, const TVector<std::pair<TString, TString>>& keyValues)
{
    auto res = TStringBuilder()
        << "/app?TabletID="
        << tabletId;
    for (const auto& p : keyValues) {
        res << "&" << p.first << "=" << p.second;
    }
    return res;
}

template <typename TRequest>
void SendUndeliverableRequest(
    TTestActorRuntimeBase& runtime,
    TAutoPtr<IEventHandle>& event,
    std::unique_ptr<TRequest> request)
{
    auto fakeRecipient = TActorId(
        event->Recipient.NodeId(),
        event->Recipient.PoolID(),
        0,
        event->Recipient.Hint());
    auto undeliveryActor = event->GetForwardOnNondeliveryRecipient();
    runtime.Send(
        new IEventHandle(
            fakeRecipient,
            event->Sender,
            request.release(),
            event->Flags,
            0,
            &undeliveryActor),
        0);
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TPartitionTest)
{
    Y_UNIT_TEST(ShouldWaitReady)
    {
        auto runtime = PrepareTestActorRuntime();

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.StatPartition();
    }

    Y_UNIT_TEST(ShouldRecoverStateOnReboot)
    {
        auto runtime = PrepareTestActorRuntime();

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.RebootTablet();

        partition.StatPartition();
    }

    Y_UNIT_TEST(ShouldStoreBlocks)
    {
        auto runtime = PrepareTestActorRuntime();

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(1, 1);
        partition.WriteBlocks(2, 2);
        partition.WriteBlocks(3, 3);

        auto response = partition.StatPartition();
        const auto& stats = response->Record.GetStats();
        UNIT_ASSERT_VALUES_EQUAL(stats.GetFreshBlocksCount(), 3);

        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(1),
            GetBlockContent(partition.ReadBlocks(1))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(2),
            GetBlockContent(partition.ReadBlocks(2))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(3),
            GetBlockContent(partition.ReadBlocks(3))
        );

        UNIT_ASSERT(stats.GetUserWriteCounters().GetExecTime() != 0);
        UNIT_ASSERT(stats.GetUserWriteCounters().GetWaitTime() != 0);
    }

    Y_UNIT_TEST(ShouldStoreBlocksInFreshChannel)
    {
        auto config = DefaultConfig();
        config.SetFreshChannelCount(1);
        config.SetFreshChannelWriteRequestsEnabled(true);

        auto runtime = PrepareTestActorRuntime(config);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(1, 1);
        partition.WriteBlocks(2, 2);
        partition.WriteBlocks(3, 3);

        auto response = partition.StatPartition();
        const auto& stats = response->Record.GetStats();
        UNIT_ASSERT_VALUES_EQUAL(stats.GetFreshBlocksCount(), 3);

        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(1),
            GetBlockContent(partition.ReadBlocks(1))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(2),
            GetBlockContent(partition.ReadBlocks(2))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(3),
            GetBlockContent(partition.ReadBlocks(3))
        );

        UNIT_ASSERT(stats.GetUserWriteCounters().GetExecTime() != 0);
        UNIT_ASSERT(stats.GetUserWriteCounters().GetWaitTime() != 0);
    }

    Y_UNIT_TEST(ShouldStoreBlocksAtTheEndOfAMaxSizeDisk)
    {
        auto config = DefaultConfig();
        config.SetWriteBlobThreshold(1_MB);
        auto runtime = PrepareTestActorRuntime(
            config,
            MaxPartitionBlocksCount
        );

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        const auto blockRange = TBlockRange32(Max<ui32>() - 500, Max<ui32>() - 2);
        partition.WriteBlocks(blockRange, 1);

        for (auto blockIndex: xrange(blockRange)) {
            UNIT_ASSERT_VALUES_EQUAL(
                GetBlockContent(1),
                GetBlockContent(partition.ReadBlocks(blockIndex))
            );
        }

        partition.WriteBlocks(Max<ui32>() - 2, 2);
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(2),
            GetBlockContent(partition.ReadBlocks(Max<ui32>() - 2))
        );

        partition.Flush();
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(2),
            GetBlockContent(partition.ReadBlocks(Max<ui32>() - 2))
        );

        partition.Compaction();
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(2),
            GetBlockContent(partition.ReadBlocks(Max<ui32>() - 2))
        );
    }

    Y_UNIT_TEST(ShouldBatchSmallWrites)
    {
        NProto::TStorageServiceConfig config;
        config.SetWriteRequestBatchingEnabled(true);
        auto runtime = PrepareTestActorRuntime(config);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        runtime->SetObserverFunc(PartitionBatchWriteCollector(1000));

        for (ui32 i = 0; i < 1000; ++i) {
            partition.SendWriteBlocksRequest(i, i);
        }

        for (ui32 i = 0; i < 1000; ++i) {
            auto response = partition.RecvWriteBlocksResponse();
            UNIT_ASSERT(SUCCEEDED(response->GetStatus()));
        }

        auto response = partition.StatPartition();
        const auto& stats = response->Record.GetStats();
        UNIT_ASSERT(stats.GetMixedBlobsCount());
        UNIT_ASSERT_VALUES_EQUAL(1000, stats.GetUsedBlocksCount());
        UNIT_ASSERT_VALUES_EQUAL(
            1000,
            stats.GetUserWriteCounters().GetRequestsCount()
        );
        const auto batchCount = stats.GetUserWriteCounters().GetBatchCount();
        UNIT_ASSERT(batchCount < 1000);
        UNIT_ASSERT(batchCount > 0);

        for (ui32 i = 0; i < 1000; ++i) {
            UNIT_ASSERT_VALUES_EQUAL(
                GetBlockContent(i),
                GetBlockContent(partition.ReadBlocks(i))
            );
        }

        UNIT_ASSERT(stats.GetUserWriteCounters().GetExecTime() != 0);
        UNIT_ASSERT(stats.GetUserWriteCounters().GetWaitTime() != 0);

        // checking that drain-related counters are in a consistent state
        partition.Drain();

        // TODO: explicitly test the case when mixed blobs are generated via batching
    }

    Y_UNIT_TEST(ShouldBatchIntersectingWrites)
    {
        NProto::TStorageServiceConfig config;
        config.SetWriteRequestBatchingEnabled(true);
        auto runtime = PrepareTestActorRuntime(config);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        runtime->SetObserverFunc(PartitionBatchWriteCollector(1000));

        for (ui32 i = 0; i < 10; ++i) {
            for (ui32 j = 0; j < 100; ++j) {
                partition.SendWriteBlocksRequest(
                    TBlockRange32(i * 100, i * 100 + j),
                    i + 1
                );
            }
        }

        for (ui32 i = 0; i < 1000; ++i) {
            auto response = partition.RecvWriteBlocksResponse();
            UNIT_ASSERT(SUCCEEDED(response->GetStatus()));
        }

        auto response = partition.StatPartition();
        const auto& stats = response->Record.GetStats();
        UNIT_ASSERT(stats.GetMixedBlobsCount());
        UNIT_ASSERT_VALUES_EQUAL(1000, stats.GetUsedBlocksCount());
        UNIT_ASSERT_VALUES_EQUAL(
            1000,
            stats.GetUserWriteCounters().GetRequestsCount()
        );
        const auto batchCount = stats.GetUserWriteCounters().GetBatchCount();
        UNIT_ASSERT(batchCount < 1000);
        UNIT_ASSERT(batchCount > 0);

        for (ui32 i = 0; i < 10; ++i) {
            for (ui32 j = 0; j < 100; ++j) {
                UNIT_ASSERT_VALUES_EQUAL(
                    GetBlockContent(i + 1),
                    GetBlockContent(partition.ReadBlocks(i * 100 + j))
                );
            }
        }

        UNIT_ASSERT(stats.GetUserWriteCounters().GetExecTime() != 0);
        UNIT_ASSERT(stats.GetUserWriteCounters().GetWaitTime() != 0);

        // checking that drain-related counters are in a consistent state
        partition.Drain();
    }

    Y_UNIT_TEST(ShouldRespectMaxBlobRangeSizeDuringBatching)
    {
        NProto::TStorageServiceConfig config = DefaultConfig();
        const auto maxBlobRangeSize = 2048;
        config.SetMaxBlobRangeSize(maxBlobRangeSize * 4_KB);
        config.SetWriteRequestBatchingEnabled(true);
        auto runtime = PrepareTestActorRuntime(config, maxBlobRangeSize * 2);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        for (ui32 i = 0; i < maxBlobRangeSize; ++i) {
            partition.SendWriteBlocksRequest(i % 2 ? i : maxBlobRangeSize + i, i);
        }

        runtime->SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvPartitionPrivate::EvAddBlobsRequest: {
                        auto* msg = event->Get<TEvPartitionPrivate::TEvAddBlobsRequest>();
                        if (msg->Mode == EAddBlobMode::ADD_WRITE_RESULT) {
                            const auto& mixedBlobs = msg->MixedBlobs;
                            for (const auto& blob: mixedBlobs) {
                                const auto blobRangeSize =
                                    blob.Blocks.back() - blob.Blocks.front();
                                Cdbg << blobRangeSize << Endl;
                                UNIT_ASSERT(blobRangeSize <= maxBlobRangeSize);
                            }
                        }

                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            }
        );

        for (ui32 i = 0; i < maxBlobRangeSize; ++i) {
            auto response = partition.RecvWriteBlocksResponse();
            UNIT_ASSERT(SUCCEEDED(response->GetStatus()));
        }

        auto response = partition.StatPartition();
        const auto& stats = response->Record.GetStats();
        UNIT_ASSERT(stats.GetMixedBlobsCount());

        for (ui32 i = 0; i < maxBlobRangeSize; ++i) {
            const auto block =
                partition.ReadBlocks(i % 2 ? i : maxBlobRangeSize + i);
            UNIT_ASSERT_VALUES_EQUAL(GetBlockContent(i), GetBlockContent(block));
        }

        UNIT_ASSERT(stats.GetUserWriteCounters().GetExecTime() != 0);
        UNIT_ASSERT(stats.GetUserWriteCounters().GetWaitTime() != 0);

        // checking that drain-related counters are in a consistent state
        partition.Drain();
    }

    Y_UNIT_TEST(ShouldStoreBlocksUsingLocalAPI)
    {
        auto runtime = PrepareTestActorRuntime();

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        auto blockContent1 = GetBlockContent(1);
        partition.WriteBlocksLocal(1, blockContent1);
        auto blockContent2 = GetBlockContent(2);
        partition.WriteBlocksLocal(2, blockContent2);
        auto blockContent3 = GetBlockContent(3);
        partition.WriteBlocksLocal(3, blockContent3);

        auto response = partition.StatPartition();
        const auto& stats = response->Record.GetStats();
        UNIT_ASSERT_VALUES_EQUAL(3, stats.GetFreshBlocksCount());

        {
            TString block(DefaultBlockSize, 0);
            partition.ReadBlocksLocal(1, block);
            UNIT_ASSERT_VALUES_EQUAL(GetBlockContent(1), block);
        }

        {
            TString block(DefaultBlockSize, 0);
            partition.ReadBlocksLocal(2, block);
            UNIT_ASSERT_VALUES_EQUAL(GetBlockContent(2), block);
        }

        {
            TString block(DefaultBlockSize, 0);
            partition.ReadBlocksLocal(3, block);
            UNIT_ASSERT_VALUES_EQUAL(GetBlockContent(3), block);
        }
    }

    Y_UNIT_TEST(ShouldRecoverBlocksOnReboot)
    {
        auto runtime = PrepareTestActorRuntime();

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(1, 1);
        partition.WriteBlocks(2, 2);
        partition.WriteBlocks(3, 3);

        auto response = partition.StatPartition();
        const auto& stats = response->Record.GetStats();
        UNIT_ASSERT_VALUES_EQUAL(stats.GetFreshBlocksCount(), 3);

        partition.RebootTablet();

        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(1),
            GetBlockContent(partition.ReadBlocks(1))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(2),
            GetBlockContent(partition.ReadBlocks(2))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(3),
            GetBlockContent(partition.ReadBlocks(3))
        );
    }

    Y_UNIT_TEST(ShouldRecoverBlocksOnRebootFromFreshChannel)
    {
        auto config = DefaultConfig();
        config.SetFreshChannelCount(1);
        config.SetFreshChannelWriteRequestsEnabled(true);

        auto runtime = PrepareTestActorRuntime(config);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(1, 1);
        partition.WriteBlocks(2, 2);
        partition.WriteBlocks(3, 3);

        auto response = partition.StatPartition();
        const auto& stats = response->Record.GetStats();
        UNIT_ASSERT_VALUES_EQUAL(3, stats.GetFreshBlocksCount());

        partition.RebootTablet();

        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(1),
            GetBlockContent(partition.ReadBlocks(1))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(2),
            GetBlockContent(partition.ReadBlocks(2))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(3),
            GetBlockContent(partition.ReadBlocks(3))
        );
    }

    Y_UNIT_TEST(ShouldFlushAsNewBlob)
    {
        auto runtime = PrepareTestActorRuntime();

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(1, 1);
        partition.WriteBlocks(2, 2);
        partition.WriteBlocks(3, 3);

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(3, stats.GetFreshBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlobsCount());
        }

        partition.Flush();

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetFreshBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(3, stats.GetMixedBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(1, stats.GetMixedBlobsCount());

            UNIT_ASSERT(stats.GetSysWriteCounters().GetExecTime() != 0);
            UNIT_ASSERT(stats.GetSysWriteCounters().GetWaitTime() != 0);
        }

        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(1),
            GetBlockContent(partition.ReadBlocks(1))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(2),
            GetBlockContent(partition.ReadBlocks(2))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(3),
            GetBlockContent(partition.ReadBlocks(3))
        );
    }

    Y_UNIT_TEST(ShouldFlushBlocksFromFreshChannelAsNewBlob)
    {
        auto config = DefaultConfig();
        config.SetFreshChannelCount(1);
        config.SetFreshChannelWriteRequestsEnabled(true);

        auto runtime = PrepareTestActorRuntime(config);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(1, 1);
        partition.WriteBlocks(2, 2);
        partition.WriteBlocks(3, 3);

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(3, stats.GetFreshBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlobsCount());
        }

        partition.Flush();

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetFreshBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(3, stats.GetMixedBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(1, stats.GetMixedBlobsCount());

            UNIT_ASSERT(stats.GetSysWriteCounters().GetExecTime() != 0);
            UNIT_ASSERT(stats.GetSysWriteCounters().GetWaitTime() != 0);
        }

        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(1),
            GetBlockContent(partition.ReadBlocks(1))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(2),
            GetBlockContent(partition.ReadBlocks(2))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(3),
            GetBlockContent(partition.ReadBlocks(3))
        );
    }

    Y_UNIT_TEST(ShouldAutomaticallyFlush)
    {
        auto config = DefaultConfig();
        config.SetWriteBlobThreshold(4_MB);

        auto runtime = PrepareTestActorRuntime(config);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(TBlockRange32(0, MaxBlocksCount - 2));

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(
                MaxBlocksCount - 1,
                stats.GetFreshBlocksCount()
            );
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlobsCount());
        }

        partition.WriteBlocks(MaxBlocksCount - 1, 0);

        // wait for background operations completion
        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetFreshBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(MaxBlocksCount, stats.GetMixedBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(1, stats.GetMixedBlobsCount());
        }
    }

    Y_UNIT_TEST(ShouldAutomaticallyFlushBlocksFromFreshChannel)
    {
        auto config = DefaultConfig();
        config.SetWriteBlobThreshold(4_MB);
        config.SetFreshChannelCount(1);
        config.SetFreshChannelWriteRequestsEnabled(true);

        auto runtime = PrepareTestActorRuntime(config);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(TBlockRange32(0, MaxBlocksCount - 2));

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(
                MaxBlocksCount - 1,
                stats.GetFreshBlocksCount()
            );
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlobsCount());
        }

        partition.WriteBlocks(MaxBlocksCount - 1, 0);

        // wait for background operations completion
        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetFreshBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(MaxBlocksCount, stats.GetMixedBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(1, stats.GetMixedBlobsCount());
        }
    }

    Y_UNIT_TEST(ShouldAutomaticallyFlushBlocksWhenFreshBlobCountThresholdIsReached)
    {
        auto config = DefaultConfig();
        config.SetWriteBlobThreshold(4_MB);
        config.SetFreshBlobCountFlushThreshold(4);
        config.SetFreshBlobByteCountFlushThreshold(999999999);
        config.SetFreshChannelCount(1);
        config.SetFreshChannelWriteRequestsEnabled(true);

        auto runtime = PrepareTestActorRuntime(config);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(TBlockRange32(0, MaxBlocksCount - 2));
        partition.WriteBlocks(TBlockRange32(0, MaxBlocksCount - 2));
        partition.WriteBlocks(TBlockRange32(0, MaxBlocksCount - 2));

        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(
                MaxBlocksCount - 1,
                stats.GetFreshBlocksCount()
            );
            UNIT_ASSERT_VALUES_EQUAL(3, stats.GetFreshBlobsCount());
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlobsCount());
        }

        partition.WriteBlocks(0, 0);

        // wait for background operations completion
        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetFreshBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetFreshBlobsCount());
            UNIT_ASSERT_VALUES_EQUAL(
                MaxBlocksCount - 1,
                stats.GetMixedBlocksCount()
            );
            UNIT_ASSERT_VALUES_EQUAL(1, stats.GetMixedBlobsCount());
        }
    }

    Y_UNIT_TEST(ShouldAutomaticallyFlushBlocksWhenFreshBlobByteCountThresholdIsReached)
    {
        auto config = DefaultConfig();
        config.SetWriteBlobThreshold(4_MB);
        config.SetFreshBlobCountFlushThreshold(999999);
        config.SetFreshBlobByteCountFlushThreshold(15_MB);
        config.SetFreshChannelCount(1);
        config.SetFreshChannelWriteRequestsEnabled(true);

        auto runtime = PrepareTestActorRuntime(config);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(TBlockRange32(0, MaxBlocksCount - 2));
        partition.WriteBlocks(TBlockRange32(0, MaxBlocksCount - 2));
        partition.WriteBlocks(TBlockRange32(0, MaxBlocksCount - 2));

        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(
                MaxBlocksCount - 1,
                stats.GetFreshBlocksCount()
            );
            UNIT_ASSERT_VALUES_EQUAL(3, stats.GetFreshBlobsCount());
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlobsCount());
        }

        partition.WriteBlocks(TBlockRange32(0, MaxBlocksCount - 2));

        // wait for background operations completion
        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetFreshBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetFreshBlobsCount());
            UNIT_ASSERT_VALUES_EQUAL(
                MaxBlocksCount - 1,
                stats.GetMixedBlocksCount()
            );
            UNIT_ASSERT_VALUES_EQUAL(1, stats.GetMixedBlobsCount());
        }
    }


    Y_UNIT_TEST(ShouldAutomaticallyTrimFreshLogOnFlush)
    {
        auto config = DefaultConfig();
        config.SetFreshChannelCount(1);
        config.SetFreshChannelWriteRequestsEnabled(true);

        auto runtime = PrepareTestActorRuntime(config);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(1, 1);
        partition.WriteBlocks(2, 2);
        partition.WriteBlocks(3, 3);

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(3, stats.GetFreshBlocksCount());
        }

        bool trimSeen = false;
        bool trimCompletedSeen = false;

        runtime->SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvBlobStorage::EvCollectGarbage: {
                        auto* msg = event->Get<TEvBlobStorage::TEvCollectGarbage>();
                        if (msg->Channel == 4) {
                            trimSeen = true;
                        }
                        break;
                    }
                    case TEvPartitionCommonPrivate::EvTrimFreshLogCompleted: {
                        auto* msg = event->Get<TEvPartitionCommonPrivate::TEvTrimFreshLogCompleted>();
                        UNIT_ASSERT(SUCCEEDED(msg->GetStatus()));
                        trimCompletedSeen = true;
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            }
        );

        partition.Flush();

        // wait for background operations completion
        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        UNIT_ASSERT_VALUES_EQUAL(true, trimSeen);
        UNIT_ASSERT_VALUES_EQUAL(true, trimCompletedSeen);
    }

    Y_UNIT_TEST(ShouldAutomaticallyTrimFreshBlobsFromPreviousGeneration)
    {
        auto config = DefaultConfig();
        config.SetFreshChannelCount(1);
        config.SetFreshChannelWriteRequestsEnabled(true);

        auto runtime = PrepareTestActorRuntime(config);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(1, 1);
        partition.WriteBlocks(2, 2);
        partition.WriteBlocks(3, 3);

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(3, stats.GetFreshBlocksCount());
        }

        bool trimSeen = false;
        bool trimCompletedSeen = false;

        runtime->SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvBlobStorage::EvCollectGarbage: {
                        auto* msg = event->Get<TEvBlobStorage::TEvCollectGarbage>();
                        if (msg->Channel == 4) {
                            trimSeen = true;
                        }
                        break;
                    }
                    case TEvPartitionCommonPrivate::EvTrimFreshLogCompleted: {
                        trimCompletedSeen = true;
                        auto* msg = event->Get<TEvPartitionCommonPrivate::TEvTrimFreshLogCompleted>();
                        UNIT_ASSERT(SUCCEEDED(msg->GetStatus()));

                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            }
        );

        partition.KillTablet();

        {
            TPartitionClient partition(*runtime);
            partition.WaitReady();

            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetFreshBlocksCount());

            UNIT_ASSERT_VALUES_EQUAL(true, trimSeen);
            UNIT_ASSERT_VALUES_EQUAL(true, trimCompletedSeen);
        }
    }

    Y_UNIT_TEST(ShouldNotAddSmallNonDeletionBlobsDuringFlush)
    {
        auto config = DefaultConfig(4_MB);
        config.SetWriteBlobThreshold(4_MB);

        auto runtime = PrepareTestActorRuntime(config, 2048);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(TBlockRange32(0, MaxBlocksCount - 2));

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(
                MaxBlocksCount - 1,
                stats.GetFreshBlocksCount()
            );
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlobsCount());
        }

        constexpr ui32 extraBlocks = 4;
        partition.WriteBlocks(TBlockRange32(MaxBlocksCount - 1, MaxBlocksCount + extraBlocks - 1));

        // wait for background operations completion
        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(extraBlocks, stats.GetFreshBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(
                MaxBlocksCount,
                stats.GetMixedBlocksCount()
            );
            UNIT_ASSERT_VALUES_EQUAL(1, stats.GetMixedBlobsCount());
        }
    }

    Y_UNIT_TEST(ShouldAddSmallBlobsDuringFlushIfThereAreAnyBlobsInFreshChannel)
    {
        auto config = DefaultConfig(4_MB);
        config.SetWriteBlobThreshold(4_MB);
        config.SetFreshChannelCount(1);
        config.SetFreshChannelWriteRequestsEnabled(true);

        auto runtime = PrepareTestActorRuntime(config, 2048);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(TBlockRange32(0, MaxBlocksCount - 2));

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(
                MaxBlocksCount - 1,
                stats.GetFreshBlocksCount()
            );
            UNIT_ASSERT_VALUES_EQUAL(1, stats.GetFreshBlobsCount());
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlobsCount());
        }

        constexpr ui32 extraBlocks = 4;
        partition.WriteBlocks(TBlockRange32(MaxBlocksCount - 1, MaxBlocksCount + extraBlocks - 1));

        // wait for background operations completion
        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetFreshBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetFreshBlobsCount());
            UNIT_ASSERT_VALUES_EQUAL(
                MaxBlocksCount + extraBlocks,
                stats.GetMixedBlocksCount()
            );
            UNIT_ASSERT_VALUES_EQUAL(2, stats.GetMixedBlobsCount());
        }
    }


    Y_UNIT_TEST(ShouldMergeVersionsOnRead)
    {
        auto runtime = PrepareTestActorRuntime();

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(1, 1);
        partition.WriteBlocks(2, 2);
        partition.WriteBlocks(3, 3);
        partition.Flush();

        partition.WriteBlocks(1, 11);
        partition.Flush();

        partition.WriteBlocks(2, 22);
        partition.Flush();

        partition.WriteBlocks(3, 33);
        // partition.Flush();

        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(11),
            GetBlockContent(partition.ReadBlocks(1))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(22),
            GetBlockContent(partition.ReadBlocks(2))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(33),
            GetBlockContent(partition.ReadBlocks(3))
        );
    }

    Y_UNIT_TEST(ShouldReplaceBlobsOnCompaction)
    {
        auto runtime = PrepareTestActorRuntime();

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(1, 1);
        partition.WriteBlocks(2, 2);
        partition.WriteBlocks(3, 3);
        partition.Flush();

        partition.WriteBlocks(1, 11);
        partition.Flush();

        partition.WriteBlocks(2, 22);
        partition.Flush();

        partition.WriteBlocks(3, 33);
        partition.Flush();

        partition.Compaction();

        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(11),
            GetBlockContent(partition.ReadBlocks(1))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(22),
            GetBlockContent(partition.ReadBlocks(2))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(33),
            GetBlockContent(partition.ReadBlocks(3))
        );

        auto response = partition.StatPartition();
        const auto& stats = response->Record.GetStats();

        UNIT_ASSERT(stats.GetSysReadCounters().GetExecTime() != 0);
        UNIT_ASSERT(stats.GetSysReadCounters().GetWaitTime() != 0);
        UNIT_ASSERT(stats.GetSysWriteCounters().GetExecTime() != 0);
        UNIT_ASSERT(stats.GetSysWriteCounters().GetWaitTime() != 0);
        UNIT_ASSERT(stats.GetUserReadCounters().GetExecTime() != 0);
        UNIT_ASSERT(stats.GetUserReadCounters().GetWaitTime() != 0);
    }

    Y_UNIT_TEST(ShouldAutomaticallyRunCompaction)
    {
        static constexpr ui32 compactionThreshold = 4;

        auto config = DefaultConfig();
        config.SetCompactionThreshold(compactionThreshold);

        auto runtime = PrepareTestActorRuntime(config);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        for (size_t i = 1; i < compactionThreshold; ++i) {
            partition.WriteBlocks(i, i);
            partition.Flush();
        }

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(
                compactionThreshold - 1,
                stats.GetMixedBlobsCount()
            );
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMergedBlobsCount());
        }

        partition.WriteBlocks(0, 0);
        partition.Flush();

        // wait for background operations completion
        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(
                compactionThreshold,
                stats.GetMixedBlobsCount()
            );
            UNIT_ASSERT_VALUES_EQUAL(1, stats.GetMergedBlobsCount());
        }
    }

    Y_UNIT_TEST(ShouldAutomaticallyRunCompactionForDeletionMarkers)
    {
        static constexpr ui32 compactionThreshold = 4;

        auto config = DefaultConfig();
        config.SetCompactionThreshold(compactionThreshold);

        auto runtime = PrepareTestActorRuntime(config);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        for (size_t i = 1; i < compactionThreshold; ++i) {
            partition.ZeroBlocks(TBlockRange32(0, 1023));
        }

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(
                compactionThreshold - 1,
                stats.GetMergedBlobsCount()
            );
        }

        partition.ZeroBlocks(TBlockRange32(0, 1023));

        // wait for background operations completion
        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(
                compactionThreshold + 1,
                stats.GetMergedBlobsCount()
            );
        }
    }

    Y_UNIT_TEST(ShouldRespectCompactionDelay)
    {
        static constexpr ui32 compactionThreshold = 4;

        auto config = DefaultConfig();
        config.SetCompactionThreshold(compactionThreshold);
        config.SetMinCompactionDelay(10000);
        config.SetMaxCompactionDelay(10000);
        config.SetCompactionScoreHistorySize(1);

        auto runtime = PrepareTestActorRuntime(config);
        runtime->AdvanceCurrentTime(TDuration::Seconds(10));

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        for (size_t i = 1; i < compactionThreshold + 1; ++i) {
            partition.WriteBlocks(i, i);
            partition.Flush();
        }

        // wait for background operations completion
        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(
                compactionThreshold,
                stats.GetMixedBlobsCount()
            );
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMergedBlobsCount());
        }

        runtime->AdvanceCurrentTime(TDuration::Seconds(10));

        // wait for background operations completion
        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(
                compactionThreshold,
                stats.GetMixedBlobsCount()
            );
            UNIT_ASSERT_VALUES_EQUAL(1, stats.GetMergedBlobsCount());
        }
    }

    Y_UNIT_TEST(ShouldRespectCleanupDelay)
    {
        auto config = DefaultConfig();
        config.SetCleanupThreshold(1);
        config.SetMinCleanupDelay(10000);
        config.SetMaxCleanupDelay(10000);
        config.SetCleanupScoreHistorySize(1);
        config.SetCollectGarbageThreshold(999999);

        auto runtime = PrepareTestActorRuntime(config);
        runtime->AdvanceCurrentTime(TDuration::Seconds(10));

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(TBlockRange32(0, 1023), 1);
        partition.WriteBlocks(TBlockRange32(0, 1023), 2);

        partition.Compaction();

        // wait for background operations completion
        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetGarbageQueueSize());
        }

        runtime->AdvanceCurrentTime(TDuration::Seconds(10));

        // wait for background operations completion
        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(2, stats.GetGarbageQueueSize());
        }
    }

    Y_UNIT_TEST(ShouldCalculateCompactionAndCleanupDelays)
    {
        static constexpr ui32 compactionThreshold = 4;
        static constexpr ui32 cleanupThreshold = 10;

        auto config = DefaultConfig();
        config.SetCompactionThreshold(compactionThreshold);
        config.SetCleanupThreshold(cleanupThreshold);
        config.SetMinCompactionDelay(0);
        config.SetMaxCompactionDelay(999'999'999);
        config.SetMinCleanupDelay(0);
        config.SetMaxCleanupDelay(999'999'999);
        config.SetMaxCompactionExecTimePerSecond(1);
        config.SetMaxCleanupExecTimePerSecond(1);

        auto runtime = PrepareTestActorRuntime(config);
        runtime->AdvanceCurrentTime(TDuration::Seconds(10));

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(TBlockRange32(0, 1023));
        // initializing compaction exec time
        partition.Compaction();

        for (size_t i = 0; i < compactionThreshold - 1; ++i) {
            partition.WriteBlocks(TBlockRange32(0, 1023));
        }

        TDuration delay;
        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(
                compactionThreshold + 1,
                stats.GetMergedBlobsCount()
            );
            delay = TDuration::MilliSeconds(stats.GetCompactionDelay());
            UNIT_ASSERT_VALUES_UNEQUAL(0, delay.MicroSeconds());
        }

        runtime->AdvanceCurrentTime(delay);

        // wait for background operations completion
        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(
                compactionThreshold + 2,
                stats.GetMergedBlobsCount()
            );
        }

        // initializing cleanup exec time
        partition.Cleanup();

        // generating enough dirty blobs for automatic cleanup
        for (ui32 i = 0; i < cleanupThreshold - 1; ++i) {
            partition.WriteBlocks(TBlockRange32(0, 1023));
        }
        partition.Compaction();

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT(stats.GetMergedBlobsCount() >= cleanupThreshold);
            delay = TDuration::MilliSeconds(stats.GetCleanupDelay());
            UNIT_ASSERT_VALUES_UNEQUAL(0, delay.MicroSeconds());
        }

        runtime->AdvanceCurrentTime(delay);

        // wait for background operations completion
        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(1, stats.GetMergedBlobsCount());
        }
    }

    Y_UNIT_TEST(ShouldCompactAndCleanupWithoutDelayUponScoreOverflow)
    {
        static constexpr ui32 compactionThreshold = 4;
        static constexpr ui32 cleanupThreshold = 10;

        auto config = DefaultConfig();
        config.SetHDDCompactionType(NProto::CT_LOAD);
        config.SetHDDMaxBlobsPerRange(compactionThreshold - 1);
        config.SetCleanupThreshold(cleanupThreshold);
        config.SetMinCompactionDelay(999'999'999);
        config.SetMaxCompactionDelay(999'999'999);
        config.SetMinCleanupDelay(999'999'999);
        config.SetMaxCleanupDelay(999'999'999);
        config.SetCompactionScoreLimitForThrottling(compactionThreshold);
        config.SetCleanupQueueBytesLimitForThrottling(16_MB);

        auto runtime = PrepareTestActorRuntime(config);
        runtime->AdvanceCurrentTime(TDuration::Seconds(10));

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        for (size_t i = 0; i < compactionThreshold; ++i) {
            partition.WriteBlocks(TBlockRange32(0, 1023));
        }
        // initializing compaction exec time
        partition.Compaction();
        // initializing cleanup exec time
        partition.Cleanup();

        for (size_t i = 0; i < compactionThreshold - 1; ++i) {
            partition.WriteBlocks(TBlockRange32(0, 1023));
        }

        // wait for background operations completion
        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(
                compactionThreshold + 1,
                stats.GetMergedBlobsCount()
            );
        }

        // generating enough dirty blobs for automatic cleanup
        for (ui32 i = 0; i < cleanupThreshold - compactionThreshold - 1; ++i) {
            partition.WriteBlocks(TBlockRange32(0, 1023));
        }
        partition.Compaction(0);

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(1, stats.GetMergedBlobsCount());
        }
    }

    Y_UNIT_TEST(ShouldAutomaticallyRunLoadOptimizingCompaction)
    {
        auto config = DefaultConfig();
        config.SetHDDCompactionType(NProto::CT_LOAD);
        config.SetHDDMaxBlobsPerRange(999);

        auto runtime = PrepareTestActorRuntime(config);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        for (ui32 i = 0; i < 512; ++i) {
            partition.WriteBlocks(TBlockRange32(i * 2, i * 2 + 1));
            partition.Flush();
        }

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(512, stats.GetMixedBlobsCount());
        }

        bool compactionRequestObserved = false;
        runtime->SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvPartitionPrivate::EvCompactionRequest: {
                        auto* msg = event->Get<TEvPartitionPrivate::TEvCompactionRequest>();
                        if (msg->Mode == TEvPartitionPrivate::RangeCompaction) {
                            compactionRequestObserved = true;
                        }
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            }
        );

        for (ui32 i = 0; i < 10; ++i) {
            for (ui32 j = 0; j < 10; ++j) {
                partition.ReadBlocks(TBlockRange32(j, j + 100));
            }
        }

        // triggering EnqueueCompactionIfNeeded(...)
        partition.WriteBlocks(0, 0);
        partition.Flush();

        // wait for background operations completion
        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        UNIT_ASSERT(compactionRequestObserved);
    }

    Y_UNIT_TEST(ShouldAutomaticallyRunGarbageCompaction)
    {
        auto config = DefaultConfig();
        config.SetHDDCompactionType(NProto::CT_LOAD);
        config.SetV1GarbageCompactionEnabled(true);
        config.SetCompactionGarbageThreshold(20);
        config.SetCompactionRangeGarbageThreshold(999999);

        auto runtime = PrepareTestActorRuntime(config, 2048);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        bool compactionRequestObserved = false;
        runtime->SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvPartitionPrivate::EvCompactionRequest: {
                        auto* msg = event->Get<TEvPartitionPrivate::TEvCompactionRequest>();
                        if (msg->Mode == TEvPartitionPrivate::GarbageCompaction) {
                            compactionRequestObserved = true;
                        }
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            }
        );

        partition.WriteBlocks(TBlockRange32(0, 1023));
        partition.WriteBlocks(TBlockRange32(0, 300));

        // wait for background operations completion
        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        UNIT_ASSERT(compactionRequestObserved);

        compactionRequestObserved = false;

        // marking range 0 as non-compacted
        partition.WriteBlocks(0);
        partition.Flush();

        partition.WriteBlocks(TBlockRange32(1024, 1400));
        // 50% garbage
        partition.WriteBlocks(TBlockRange32(1024, 1400));

        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        UNIT_ASSERT(compactionRequestObserved);

        compactionRequestObserved = false;

        partition.CreateCheckpoint("c1");

        // writing lots of blocks into range 1
        partition.WriteBlocks(TBlockRange32(1024, 2047));
        partition.WriteBlocks(TBlockRange32(1024, 2047));

        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        // there is a checkpoint => garbage-based compaction should not run
        UNIT_ASSERT(!compactionRequestObserved);

        partition.DeleteCheckpoint("c1");

        // triggering compaction attempt, block index does not matter
        partition.WriteBlocks(0);
        partition.Flush();

        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));
        UNIT_ASSERT(compactionRequestObserved);
    }

    Y_UNIT_TEST(ShouldAutomaticallyRunGarbageCompactionForSuperDirtyRanges)
    {
        auto config = DefaultConfig();
        config.SetHDDCompactionType(NProto::CT_LOAD);
        config.SetV1GarbageCompactionEnabled(true);
        config.SetCompactionGarbageThreshold(999999);
        config.SetCompactionRangeGarbageThreshold(200);

        auto runtime = PrepareTestActorRuntime(config);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        bool compactionRequestObserved = false;
        runtime->SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvPartitionPrivate::EvCompactionRequest: {
                        auto* msg = event->Get<TEvPartitionPrivate::TEvCompactionRequest>();
                        if (msg->Mode == TEvPartitionPrivate::GarbageCompaction) {
                            compactionRequestObserved = true;
                        }
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            }
        );

        partition.WriteBlocks(TBlockRange32(0, 1023));
        partition.WriteBlocks(TBlockRange32(0, 1023));
        partition.WriteBlocks(TBlockRange32(0, 1023));

        // wait for background operations completion
        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        // garbage == used x2 => compaction
        UNIT_ASSERT(compactionRequestObserved);

        compactionRequestObserved = false;

        partition.WriteBlocks(TBlockRange32(0, 1023));

        // wait for background operations completion
        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        // garbage == used x1 => no compaction
        UNIT_ASSERT(!compactionRequestObserved);

        partition.CreateCheckpoint("c1");

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(1024, stats.GetCompactionGarbageScore());
        }

        partition.WriteBlocks(TBlockRange32(0, 1023));

        // wait for background operations completion
        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        // garbage == used x2 => still no compaction because there is a checkpoint
        UNIT_ASSERT(!compactionRequestObserved);

        partition.DeleteCheckpoint("c1");

        partition.WriteBlocks(TBlockRange32(0, 0));
        partition.Flush();

        // wait for background operations completion
        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        // garbage == used x2 => compaction
        UNIT_ASSERT(compactionRequestObserved);

        compactionRequestObserved = false;

        partition.WriteBlocks(TBlockRange32(0, 0));
        partition.Flush();

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(1, stats.GetCompactionGarbageScore());
        }

        // wait for background operations completion
        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        // block count for this range should've been reset
        UNIT_ASSERT(!compactionRequestObserved);

        // a range with no used blocks
        partition.WriteBlocks(TBlockRange32(0, 1023));
        partition.ZeroBlocks(TBlockRange32(0, 1023));

        // wait for background operations completion
        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        // only garbage => compaction
        UNIT_ASSERT(compactionRequestObserved);

        compactionRequestObserved = false;

        partition.WriteBlocks(TBlockRange32(0, 100));
        partition.Flush();

        // wait for background operations completion
        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        // no garbage (99% of the blocks belong to a deletion marker) => no compaction
        UNIT_ASSERT(!compactionRequestObserved);

        partition.WriteBlocks(TBlockRange32(0, 100));
        partition.Flush();

        // wait for background operations completion
        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        // garbage == used => no compaction
        UNIT_ASSERT(!compactionRequestObserved);

        partition.WriteBlocks(TBlockRange32(0, 100));
        partition.Flush();

        // wait for background operations completion
        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        // garbage == 2 * used => compaction
        UNIT_ASSERT(compactionRequestObserved);
    }

    Y_UNIT_TEST(CompactionShouldTakeCareOfFreshBlocks)
    {
        auto runtime = PrepareTestActorRuntime();

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(1, 1);
        partition.WriteBlocks(2, 2);
        partition.WriteBlocks(3, 3);
        partition.Flush();

        partition.WriteBlocks(1, 11);
        partition.Flush();

        partition.WriteBlocks(2, 22);
        partition.Flush();

        partition.WriteBlocks(3, 33);
        // partition.Flush();

        partition.Compaction();

        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(11),
            GetBlockContent(partition.ReadBlocks(1))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(22),
            GetBlockContent(partition.ReadBlocks(2))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(33),
            GetBlockContent(partition.ReadBlocks(3))
        );
    }

    Y_UNIT_TEST(ShouldZeroBlocks)
    {
        auto runtime = PrepareTestActorRuntime();

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(1, 1);
        partition.WriteBlocks(2, 2);
        partition.WriteBlocks(3, 3);

        partition.Flush();
        partition.ZeroBlocks(2);

        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(1),
            GetBlockContent(partition.ReadBlocks(1))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            "",
            GetBlockContent(partition.ReadBlocks(2))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(3),
            GetBlockContent(partition.ReadBlocks(3))
        );

        partition.Flush();
        partition.ZeroBlocks(3);

        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(1),
            GetBlockContent(partition.ReadBlocks(1))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            "",
            GetBlockContent(partition.ReadBlocks(2))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            "",
            GetBlockContent(partition.ReadBlocks(3))
        );
    }

    Y_UNIT_TEST(ShouldZeroBlocksWrittenToFreshChannelAfterReboot)
    {
        auto config = DefaultConfig();
        config.SetFreshChannelCount(1);
        config.SetFreshChannelWriteRequestsEnabled(true);

        auto runtime = PrepareTestActorRuntime(config);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(1, 1);
        partition.ZeroBlocks(1);

        partition.RebootTablet();

        UNIT_ASSERT_VALUES_EQUAL(
            "",
            GetBlockContent(partition.ReadBlocks(1))
        );
    }

    Y_UNIT_TEST(ShouldReadZeroFromUninitializedBlock)
    {
        auto runtime = PrepareTestActorRuntime();

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(1, 1);
        // partition.WriteBlocks(2, 2);
        partition.WriteBlocks(3, 3);

        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(1),
            GetBlockContent(partition.ReadBlocks(1))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            "",
            GetBlockContent(partition.ReadBlocks(2))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(3),
            GetBlockContent(partition.ReadBlocks(3))
        );

        partition.Flush();

        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(1),
            GetBlockContent(partition.ReadBlocks(1))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            "",
            GetBlockContent(partition.ReadBlocks(2))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(3),
            GetBlockContent(partition.ReadBlocks(3))
        );
    }

    Y_UNIT_TEST(ShouldZeroLargeNumberOfBlocks)
    {
        auto runtime = PrepareTestActorRuntime();

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(TBlockRange32(0, MaxBlocksCount - 1));
        partition.ZeroBlocks(TBlockRange32(0, MaxBlocksCount - 1));

        UNIT_ASSERT_VALUES_EQUAL(
            "",
            GetBlockContent(partition.ReadBlocks(0))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            "",
            GetBlockContent(partition.ReadBlocks(MaxBlocksCount / 2))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            "",
            GetBlockContent(partition.ReadBlocks(MaxBlocksCount - 1))
        );
    }

    Y_UNIT_TEST(ShouldHandleZeroedBlocksInCompaction)
    {
        auto runtime = PrepareTestActorRuntime();

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(TBlockRange32(0, MaxBlocksCount - 1));
        partition.Flush();

        partition.ZeroBlocks(TBlockRange32(0, MaxBlocksCount - 1));
        partition.Flush();

        partition.Compaction();
    }

    Y_UNIT_TEST(ShouldCleanupBlobs)
    {
        auto runtime = PrepareTestActorRuntime();

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlobsCount());
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMergedBlobsCount());
        }

        partition.WriteBlocks(TBlockRange32(0, MaxBlocksCount - 1), 1);

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlobsCount());
            UNIT_ASSERT_VALUES_EQUAL(1, stats.GetMergedBlobsCount());
        }

        partition.WriteBlocks(TBlockRange32(0, MaxBlocksCount - 1), 2);

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlobsCount());
            UNIT_ASSERT_VALUES_EQUAL(2, stats.GetMergedBlobsCount());
        }

        partition.Compaction();

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlobsCount());
            UNIT_ASSERT_VALUES_EQUAL(3, stats.GetMergedBlobsCount());
        }

        partition.Cleanup();

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlobsCount());
            UNIT_ASSERT_VALUES_EQUAL(1, stats.GetMergedBlobsCount());
        }
    }

    Y_UNIT_TEST(ShouldReadFromCheckpoint)
    {
        auto runtime = PrepareTestActorRuntime();

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(1, 1);
        partition.CreateCheckpoint("checkpoint1");

        partition.Flush();
        partition.WriteBlocks(1, 2);
        partition.CreateCheckpoint("checkpoint2");

        partition.Flush();
        partition.WriteBlocks(1, 3);
        partition.CreateCheckpoint("checkpoint3");

        partition.WriteBlocks(1, 4);

        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(1),
            GetBlockContent(partition.ReadBlocks(1, "checkpoint1"))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(2),
            GetBlockContent(partition.ReadBlocks(1, "checkpoint2"))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(3),
            GetBlockContent(partition.ReadBlocks(1, "checkpoint3"))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(4),
            GetBlockContent(partition.ReadBlocks(1))
        );
    }

    Y_UNIT_TEST(ShouldKillTabletIfCriticalFailureDuringWriteBlocks)
    {
        auto runtime = PrepareTestActorRuntime();

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        runtime->SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvPartitionPrivate::EvWriteBlobResponse: {
                        auto* msg = event->Get<TEvPartitionPrivate::TEvWriteBlobResponse>();
                        auto& e = const_cast<NProto::TError&>(msg->Error);
                        e.SetCode(E_REJECTED);
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        partition.SendWriteBlocksRequest(TBlockRange32(0, MaxBlocksCount - 1));
        auto response = partition.RecvWriteBlocksResponse();

        UNIT_ASSERT(FAILED(response->GetStatus()));
    }

    Y_UNIT_TEST(ShouldKillTabletIfCriticalFailureDuringFlush)
    {
        auto runtime = PrepareTestActorRuntime();

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(TBlockRange32(0, 9));
        partition.WriteBlocks(TBlockRange32(10, 19));
        partition.WriteBlocks(TBlockRange32(20, 29));

        int pillCount = 0;

        runtime->SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvents::TSystem::PoisonPill: {
                        ++pillCount;
                        break;
                    }
                    case TEvPartitionPrivate::EvWriteBlobResponse: {
                        auto* msg = event->Get<TEvPartitionPrivate::TEvWriteBlobResponse>();
                        auto& e = const_cast<NProto::TError&>(msg->Error);
                        e.SetCode(E_REJECTED);
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        partition.SendFlushRequest();
        auto response = partition.RecvFlushResponse();

        UNIT_ASSERT(FAILED(response->GetStatus()));
    }

    auto BuildEvGetBreaker(ui32 blockCount, bool& broken) {
        return [blockCount, &broken] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
            switch (event->GetTypeRewrite()) {
                case TEvBlobStorage::EvGetResult: {
                    auto* msg = event->Get<TEvBlobStorage::TEvGetResult>();
                    ui32 totalSize = 0;
                    for (ui32 i = 0; i < msg->ResponseSz; ++i) {
                        totalSize += msg->Responses[i].Buffer.size();
                    }
                    if (totalSize == blockCount * DefaultBlockSize) {
                        // it's our blob
                        msg->Responses[0].Status = NKikimrProto::NODATA;
                        broken = true;
                    }

                    break;
                }
            }

            return TTestActorRuntime::DefaultObserverFunc(runtime, event);
        };
    }

    Y_UNIT_TEST(ShouldReturnErrorIfNODATABlocksAreDetectedInDefaultStorageAccessMode)
    {
        auto runtime = PrepareTestActorRuntime(
            DefaultConfig(),
            1024,
            {},
            TTestPartitionInfo(),
            {},
            EStorageAccessMode::Default
        );

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        bool broken = false;
        runtime->SetObserverFunc(BuildEvGetBreaker(777, broken));

        partition.WriteBlocks(TBlockRange32(0, 776), 1);
        partition.SendReadBlocksRequest(TBlockRange32(0, 776));
        auto response = partition.RecvReadBlocksResponse();
        Y_VERIFY(broken);
        UNIT_ASSERT_VALUES_EQUAL(E_REJECTED, response->GetError().GetCode());
    }

    Y_UNIT_TEST(ShouldMarkNODATABlocksInRepairStorageAccessMode)
    {
        auto runtime = PrepareTestActorRuntime(
            DefaultConfig(),
            1024,
            {},
            TTestPartitionInfo(),
            {},
            EStorageAccessMode::Repair
        );

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        bool broken = false;
        runtime->SetObserverFunc(BuildEvGetBreaker(777, broken));

        partition.WriteBlocks(TBlockRange32(0, 776), 1);
        partition.SendReadBlocksRequest(TBlockRange32(0, 776));
        auto response = partition.RecvReadBlocksResponse();
        Y_VERIFY(broken);
        UNIT_ASSERT_VALUES_EQUAL(S_OK, response->GetError().GetCode());

        const auto& blocks = response->Record.GetBlocks();
        UNIT_ASSERT_EQUAL(777, blocks.BuffersSize());
        for (size_t i = 0; i < blocks.BuffersSize(); ++i) {
            UNIT_ASSERT_VALUES_EQUAL(
                GetBrokenDataMarker(),
                blocks.GetBuffers(i)
            );
        }
    }

    Y_UNIT_TEST(ShouldRejectRequestForBlockOutOfRange)
    {
        auto runtime = PrepareTestActorRuntime();

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        {
            partition.WriteBlocks(TBlockRange32(1023, 1023));
        }

        {
            partition.SendWriteBlocksRequest(TBlockRange32(1023, 1024));
            auto response = partition.RecvWriteBlocksResponse();
            UNIT_ASSERT(FAILED(response->GetStatus()));
        }

        {
            partition.SendWriteBlocksRequest(TBlockRange32(1024, 1024));
            auto response = partition.RecvWriteBlocksResponse();
            UNIT_ASSERT(FAILED(response->GetStatus()));
        }
    }

    Y_UNIT_TEST(ShouldNotCauseUI32IntergerOverflow)
    {
        auto runtime = PrepareTestActorRuntime(DefaultConfig(), Max<ui32>());

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        {
            auto range = TBlockRange32(Max<ui32>() - 1, Max<ui32>() - 1);
            partition.WriteBlocks(range);
            partition.ReadBlocks(Max<ui32>() - 1);
        }

        {
            auto range = TBlockRange32(Max<ui32>() - 1, Max<ui32>());
            partition.SendWriteBlocksRequest(range);
            auto response = partition.RecvWriteBlocksResponse();
            UNIT_ASSERT(FAILED(response->GetStatus()));
        }

        {
            auto range = TBlockRange32(Max<ui32>() - 1, Max<ui32>() - 1);
            auto blockContent = GetBlockContent(1);
            partition.WriteBlocksLocal(range, blockContent);
            partition.ReadBlocks(Max<ui32>() - 1);
        }

        {
            auto range = TBlockRange32(Max<ui32>() - 1, Max<ui32>());
            auto blockContent = GetBlockContent(1);
            partition.SendWriteBlocksLocalRequest(range, blockContent);
            auto response = partition.RecvWriteBlocksLocalResponse();
            UNIT_ASSERT(FAILED(response->GetStatus()));
        }

        {
            auto range = TBlockRange32(Max<ui32>() - 1, Max<ui32>());
            TVector<TString> blocks;
            auto sglist = ResizeBlocks(
                blocks,
                range.Size(),
                TString::TUninitialized(DefaultBlockSize));
            partition.SendReadBlocksLocalRequest(range, std::move(sglist));
            auto response = partition.RecvReadBlocksLocalResponse();
            UNIT_ASSERT(FAILED(response->GetStatus()));
        }
    }

    Y_UNIT_TEST(ShouldSupportCheckpointOperations)
    {
        auto runtime = PrepareTestActorRuntime(DefaultConfig());

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.CreateCheckpoint("checkpoint1");
        partition.DeleteCheckpoint("checkpoint1");

        partition.CreateCheckpoint("checkpoint1");
        partition.DeleteCheckpoint("checkpoint1");
        partition.DeleteCheckpoint("checkpoint1");

        partition.CreateCheckpoint("checkpoint1", "id1");
        partition.DeleteCheckpoint("checkpoint1", "id1");

        partition.CreateCheckpoint("checkpoint1", "id1");
        partition.CreateCheckpoint("checkpoint1", "id1");
        partition.DeleteCheckpoint("checkpoint1", "id2");

        partition.CreateCheckpoint("checkpoint1", "id1");
        partition.DeleteCheckpoint("checkpoint1", "id2");
        partition.DeleteCheckpoint("checkpoint1", "id2");

        partition.CreateCheckpoint("checkpoint1", "id1");
        partition.DeleteCheckpoint("checkpoint1", "id2");
        partition.DeleteCheckpoint("checkpoint1", "id3");

        partition.CreateCheckpoint("checkpoint1", "id1");
        partition.CreateCheckpoint("checkpoint1", "id2");
        partition.DeleteCheckpoint("checkpoint1", "id2");

        partition.CreateCheckpoint("checkpoint1", "id1");
        partition.DeleteCheckpoint("checkpoint2", "id1");
    }

    Y_UNIT_TEST(ShouldDeleteCheckpointAfterDeleteCheckpointData)
    {
        auto runtime = PrepareTestActorRuntime(DefaultConfig());

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.CreateCheckpoint("checkpoint1");
        partition.DeleteCheckpointData("checkpoint1");

        auto responseDelete = partition.DeleteCheckpoint("checkpoint1");
        UNIT_ASSERT_VALUES_EQUAL(S_OK, responseDelete->GetStatus());
    }

    Y_UNIT_TEST(ShouldDeleteCheckpointAfterDeleteCheckpointDataAndReboot)
    {
        auto runtime = PrepareTestActorRuntime(DefaultConfig());

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(1, 1);
        partition.CreateCheckpoint("checkpoint1");
        partition.Flush();
        partition.WriteBlocks(1, 2);

        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(1),
            GetBlockContent(partition.ReadBlocks(1, "checkpoint1"))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(2),
            GetBlockContent(partition.ReadBlocks(1))
        );

        partition.CreateCheckpoint("checkpoint1");
        partition.DeleteCheckpointData("checkpoint1");

        partition.SendReadBlocksRequest(1, "checkpoint1");
        auto response = partition.RecvReadBlocksResponse();
        UNIT_ASSERT_VALUES_EQUAL(E_NOT_FOUND, response->GetStatus());

        partition.RebootTablet();
        partition.WaitReady();

        partition.SendReadBlocksRequest(1, "checkpoint1");
        response = partition.RecvReadBlocksResponse();
        UNIT_ASSERT_VALUES_EQUAL(E_NOT_FOUND, response->GetStatus());

        {
            auto responseDelete = partition.DeleteCheckpoint("checkpoint1");
            UNIT_ASSERT_VALUES_EQUAL(S_OK, responseDelete->GetStatus());
        }
        {
            auto responseCreate = partition.CreateCheckpoint("checkpoint1");
            UNIT_ASSERT_VALUES_EQUAL(S_OK, responseCreate->GetStatus());
        }
        {
            auto responseDelete = partition.DeleteCheckpoint("checkpoint1");
            UNIT_ASSERT_VALUES_EQUAL(S_OK, responseDelete->GetStatus());
        }
    }

    Y_UNIT_TEST(ShouldCreateBlobsForEveryWrittenRangeDuringForcedCompaction)
    {
        constexpr ui32 rangesCount = 5;
        auto runtime = PrepareTestActorRuntime(DefaultConfig(), rangesCount * 1024);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        for (ui32 range = 0; range < rangesCount; ++range) {
            partition.WriteBlocks(
                TBlockRange32(range * 1024 + 100, range * 1024 + 1000),
                1);
        }
        partition.Flush();

        auto response = partition.StatPartition();
        auto oldStats = response->Record.GetStats();

        for (ui32 range = 0; range < rangesCount; ++range) {
            partition.Compaction(range * 1024);
        }

        response = partition.StatPartition();
        auto newStats = response->Record.GetStats();
        UNIT_ASSERT_VALUES_EQUAL(
            oldStats.GetMixedBlobsCount() + oldStats.GetMergedBlobsCount() + rangesCount,
            newStats.GetMixedBlobsCount() + newStats.GetMergedBlobsCount()
        );
    }

    Y_UNIT_TEST(ShouldNotCreateBlobsForEmptyRangesDuringForcedCompaction)
    {
        constexpr ui32 rangesCount = 5;
        constexpr ui32 emptyRange = 2;
        auto runtime = PrepareTestActorRuntime(DefaultConfig(), rangesCount * 1024);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        for (ui32 range = 0; range < rangesCount; ++range) {
            if (range != emptyRange) {
                partition.WriteBlocks(
                    TBlockRange32(range * 1024 + 100, range * 1024 + 1000),
                    1);
            }
        }
        partition.Flush();

        auto response = partition.StatPartition();
        auto oldStats = response->Record.GetStats();

        for (ui32 range = 0; range < rangesCount; ++range) {
            partition.Compaction(range * 1024);
        }

        response = partition.StatPartition();
        auto newStats = response->Record.GetStats();
        UNIT_ASSERT_VALUES_EQUAL(
            oldStats.GetMixedBlobsCount() + oldStats.GetMergedBlobsCount() + rangesCount - 1,
            newStats.GetMixedBlobsCount() + newStats.GetMergedBlobsCount()
        );
    }

    Y_UNIT_TEST(ShouldCorrectlyMarkFirstBlockInBlobIfItIsTheSameAsLastBlockInPreviousBlob)
    {
        auto config = DefaultConfig();
        config.SetCleanupThreshold(1);
        config.SetCollectGarbageThreshold(1);
        config.SetFlushThreshold(8_MB);
        config.SetWriteBlobThreshold(4_MB);
        config.SetWriteMergedBlobThreshold(4_MB);

        auto runtime = PrepareTestActorRuntime(config, 2048);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(TBlockRange32(0,1022), 1);
        for (ui32 i = 0; i < 1024; ++i) {
            partition.WriteBlocks(1023,1);
        }

        // here we expect that fresh contains 2048 blocks
        // flush will write two blobs but latest blob
        // will be immediately gc as it is completely overwritten

        partition.Flush();

        auto response = partition.StatPartition();
        const auto& stats = response->Record.GetStats();
        UNIT_ASSERT_VALUES_EQUAL(1, stats.GetMixedBlobsCount());
    }

    Y_UNIT_TEST(ShouldMakeUnderlyingBlobsEligibleForCleanupAfterCompaction)
    {
        auto config = DefaultConfig();
        config.SetCleanupThreshold(1);
        config.SetCollectGarbageThreshold(3);
        config.SetCompactionThreshold(4);
        config.SetFlushThreshold(8_MB);
        config.SetWriteBlobThreshold(4_MB);
        config.SetWriteMergedBlobThreshold(4_MB);

        auto runtime = PrepareTestActorRuntime(config, 2048);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        for (ui32 version = 0; version < 3; ++version) {
            for (ui32 pos = 0; pos < 2; ++pos) {
                partition.WriteBlocks(TBlockRange32::WithLength(512 * pos, 512));
            }
            partition.Flush();
        }

        partition.Compaction();
        partition.Cleanup();
        partition.CollectGarbage();

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(1, stats.GetMergedBlobsCount());
        }
    }

    Y_UNIT_TEST(ShouldCorrectlyMarkCrossRangeBlocksDuringCompaction)
    {
        auto config = DefaultConfig();
        config.SetCleanupThreshold(1);
        config.SetCollectGarbageThreshold(3);
        config.SetCompactionThreshold(4);
        config.SetFlushThreshold(8_MB);
        config.SetWriteBlobThreshold(4_MB);
        config.SetWriteMergedBlobThreshold(4_MB);

        auto runtime = PrepareTestActorRuntime(config, 2048);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        for (ui32 version = 0; version < 2; ++version) {
            for (ui32 pos = 0; pos < 4; ++pos) {
                partition.WriteBlocks(TBlockRange32::WithLength(512 * pos, 512));
            }
            partition.Flush();
        }

        partition.WriteBlocks(TBlockRange32(1000, 1500));
        partition.Flush();

        partition.Compaction();
        partition.Compaction();
        partition.Cleanup();
        partition.CollectGarbage();

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(2, stats.GetMergedBlobsCount());
        }
    }

    Y_UNIT_TEST(ShouldNotWriteBlobIfCompactionRangeWasOverwrittenWithZeroBlocks)
    {
        auto config = DefaultConfig();
        config.SetCleanupThreshold(1);
        config.SetCompactionThreshold(2);
        config.SetCollectGarbageThreshold(1);
        config.SetFlushThreshold(8_MB);
        config.SetWriteBlobThreshold(4_MB);
        config.SetWriteMergedBlobThreshold(4_MB);

        auto runtime = PrepareTestActorRuntime(config, 2048);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        for (ui32 write_step = 0; write_step < 2; ++write_step) {
            partition.WriteBlocks(TBlockRange32(0,511));
        }

        partition.Flush();

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(1, stats.GetMixedBlobsCount());
        }

        for (ui32 zero_step = 0; zero_step < 2; ++zero_step) {
            partition.ZeroBlocks(TBlockRange32(0,511));
        }

        bool writeBlobSeen = false;
        runtime->SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvPartitionPrivate::EvWriteBlobRequest: {
                        writeBlobSeen = true;
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        partition.Compaction();
        partition.Cleanup();
        partition.CollectGarbage();

        UNIT_ASSERT(!writeBlobSeen);
        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlobsCount());
        }
    }

    Y_UNIT_TEST(ShouldWriteBlobIfCompactionUsesOnlyFreshBlocks)
    {
        auto config = DefaultConfig();
        config.SetCleanupThreshold(3);
        config.SetCompactionThreshold(2);
        config.SetCollectGarbageThreshold(3);
        config.SetFlushThreshold(8_MB);
        config.SetWriteBlobThreshold(4_MB);
        config.SetWriteMergedBlobThreshold(4_MB);

        auto runtime = PrepareTestActorRuntime(config, 2048);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        for (ui32 write_step = 0; write_step < 2; ++write_step) {
            partition.WriteBlocks(TBlockRange32(0,511));
        }

        partition.Flush();

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(1, stats.GetMixedBlobsCount());
        }

        for (ui32 write_step = 0; write_step < 2; ++write_step) {
            partition.WriteBlocks(TBlockRange32(0,511));
        }

        bool writeBlobSeen = false;
        runtime->SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvPartitionPrivate::EvWriteBlobRequest: {
                        writeBlobSeen = true;
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        partition.Compaction();

        UNIT_ASSERT(writeBlobSeen);
        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(1, stats.GetMixedBlobsCount());
            UNIT_ASSERT_VALUES_EQUAL(1, stats.GetMergedBlobsCount());
        }
    }

    Y_UNIT_TEST(ShouldCorrectlyHandleZeroAndNonZeroFreshBlobsDuringFlush)
    {
        auto config = DefaultConfig();
        config.SetFlushThreshold(12_KB);
        config.SetFlushBlobSizeThreshold(8_KB);

        auto runtime = PrepareTestActorRuntime(config, 2048);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(0, 1);
        partition.ZeroBlocks(0);
        partition.WriteBlocks(0, 1);

        partition.Flush();

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(1, stats.GetMixedBlobsCount());
        }
    }

    Y_UNIT_TEST(ShouldRejectWriteRequestsIfDataChannelsAreYellow)
    {
        auto config = DefaultConfig();

        auto runtime = PrepareTestActorRuntime(config, 2048);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        runtime->SetObserverFunc(StorageStateChanger(
            NKikimrBlobStorage::StatusDiskSpaceLightYellowMove |
            NKikimrBlobStorage::StatusDiskSpaceYellowStop));

        partition.WriteBlocks(TBlockRange32(0, 1023));

        {
            partition.SendWriteBlocksRequest(TBlockRange32(0, 1023));
            auto response = partition.RecvWriteBlocksResponse();
            UNIT_ASSERT(FAILED(response->GetStatus()));
        }
    }

    Y_UNIT_TEST(ShouldCorrectlyTrackYellowChannelsInBackground)
    {
        const auto channelCount = 6;
        const auto groupCount = channelCount - DataChannelOffset;

        TTestEnv env(0, 1, channelCount, groupCount);
        auto& runtime = env.GetRuntime();
        auto tabletId = InitTestActorRuntime(env, runtime, channelCount, channelCount);

        TPartitionClient partition(runtime, 0, tabletId);
        partition.WaitReady();

        // one data channel yellow => ok
        {
            auto request =
                std::make_unique<TEvTablet::TEvCheckBlobstorageStatusResult>(
                    TVector<ui32>({
                        env.GetGroupIds()[1],
                    }),
                    TVector<ui32>({
                        env.GetGroupIds()[1],
                    })
                );
            partition.SendToPipe(std::move(request));
        }

        runtime.DispatchEvents({},  TDuration::Seconds(1));

        {
            partition.SendWriteBlocksRequest(TBlockRange32(0, 1023));
            auto response = partition.RecvWriteBlocksResponse();
            UNIT_ASSERT(SUCCEEDED(response->GetStatus()));
        }

        // all channels yellow => failure
        {
            auto request =
                std::make_unique<TEvTablet::TEvCheckBlobstorageStatusResult>(
                    TVector<ui32>({
                        env.GetGroupIds()[0],
                        env.GetGroupIds()[1],
                    }),
                    TVector<ui32>({
                        env.GetGroupIds()[0],
                        env.GetGroupIds()[1],
                    })
                );
            partition.SendToPipe(std::move(request));
        }

        runtime.DispatchEvents({},  TDuration::Seconds(1));

        {
            partition.SendWriteBlocksRequest(TBlockRange32(0, 1023));
            auto response = partition.RecvWriteBlocksResponse();
            UNIT_ASSERT(FAILED(response->GetStatus()));
        }

        // channels returned to non-yellow state => ok
        {
            auto request =
                std::make_unique<TEvTablet::TEvCheckBlobstorageStatusResult>(
                    TVector<ui32>(),
                    TVector<ui32>()
                );
            partition.SendToPipe(std::move(request));
        }

        runtime.DispatchEvents({},  TDuration::Seconds(1));

        {
            partition.SendWriteBlocksRequest(TBlockRange32(0, 1023));
            auto response = partition.RecvWriteBlocksResponse();
            UNIT_ASSERT(SUCCEEDED(response->GetStatus()));
        }

        // TODO: the state may be neither yellow nor green (e.g. orange) -
        // this case is not supported in the background check, but should be
    }

    Y_UNIT_TEST(ShouldSeeChangedFreshBlocksInChangedBlocksRequest)
    {
        auto config = DefaultConfig();

        auto runtime = PrepareTestActorRuntime(config, 2048);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(0,1);
        partition.WriteBlocks(1,1);
        partition.CreateCheckpoint("cp1");

        partition.WriteBlocks(1,2);
        partition.WriteBlocks(2,2);
        partition.CreateCheckpoint("cp2");

        auto response = partition.GetChangedBlocks(TBlockRange32(0, 1023), "cp1", "cp2");

        UNIT_ASSERT_VALUES_EQUAL(128, response->Record.GetMask().size());
        UNIT_ASSERT_VALUES_EQUAL(6, response->Record.GetMask()[0]);
    }

    Y_UNIT_TEST(ShouldSeeChangedMergedBlocksInChangedBlocksRequest)
    {
        auto config = DefaultConfig();

        auto runtime = PrepareTestActorRuntime(config, 4096);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(TBlockRange32(0, 1023), 1);
        partition.WriteBlocks(TBlockRange32(1024, 2047), 1);
        partition.CreateCheckpoint("cp1");

        partition.WriteBlocks(TBlockRange32(1024, 2047), 2);
        partition.WriteBlocks(TBlockRange32(2048, 3071), 2);
        partition.CreateCheckpoint("cp2");

        auto response = partition.GetChangedBlocks(TBlockRange32(0, 3071), "cp1", "cp2");

        const auto& mask = response->Record.GetMask();
        UNIT_ASSERT_VALUES_EQUAL(384, mask.size());
        AssertEqual(
            TVector<ui8>(256, 255),
            TVector<ui8>(mask.begin() + 128, mask.end())
        );
    }

    Y_UNIT_TEST(ShouldUseZeroCommitIdWhenLowCheckpointIdIsNotSet)
    {
        auto config = DefaultConfig();

        auto runtime = PrepareTestActorRuntime(config, 4096);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(0, 1);
        partition.CreateCheckpoint("cp1");
        partition.WriteBlocks(1, 1);
        partition.CreateCheckpoint("cp2");

        {
            auto response = partition.GetChangedBlocks(TBlockRange32(0, 3071), "cp1", "cp2");

            const auto& mask = response->Record.GetMask();
            UNIT_ASSERT_VALUES_EQUAL(384, mask.size());
            UNIT_ASSERT_VALUES_EQUAL(2, response->Record.GetMask()[0]);
        }
        {
            auto response = partition.GetChangedBlocks(TBlockRange32(0, 3071), "", "cp2");

            const auto& mask = response->Record.GetMask();
            UNIT_ASSERT_VALUES_EQUAL(384, mask.size());
            UNIT_ASSERT_VALUES_EQUAL(3, response->Record.GetMask()[0]);
        }
    }

    Y_UNIT_TEST(ShouldUseMostRecentCommitIdWhenHighCheckpointIdIsNotSet)
    {
        auto config = DefaultConfig();

        auto runtime = PrepareTestActorRuntime(config, 4096);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(0, 1);
        partition.WriteBlocks(1, 1);
        partition.CreateCheckpoint("cp1");

        partition.WriteBlocks(1, 1);

        partition.CreateCheckpoint("cp2");

        partition.WriteBlocks(2, 1);

        {
            auto response = partition.GetChangedBlocks(TBlockRange32(0, 3071), "cp1", "cp2");

            const auto& mask = response->Record.GetMask();
            UNIT_ASSERT_VALUES_EQUAL(384, mask.size());
            UNIT_ASSERT_VALUES_EQUAL(2, response->Record.GetMask()[0]);
        }

        {
            auto response = partition.GetChangedBlocks(TBlockRange32(0, 3071), "cp1", "");

            const auto& mask = response->Record.GetMask();
            UNIT_ASSERT_VALUES_EQUAL(384, mask.size());
            UNIT_ASSERT_VALUES_EQUAL(6, response->Record.GetMask()[0]);
        }
    }

    Y_UNIT_TEST(ShouldCorrectlyHandleStartBlockInChangedBlocksRequest)
    {
        auto config = DefaultConfig();

        auto runtime = PrepareTestActorRuntime(config, 4096);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(0,1);
        partition.CreateCheckpoint("cp1");

        partition.WriteBlocks(0,1);
        partition.WriteBlocks(1024,1);
        partition.CreateCheckpoint("cp2");

        {
            auto response = partition.GetChangedBlocks(TBlockRange32(0, 1023), "cp1", "cp2");

            const auto& mask = response->Record.GetMask();
            UNIT_ASSERT_VALUES_EQUAL(128, mask.size());
            UNIT_ASSERT_VALUES_EQUAL(1, mask[0]);
            AssertEqual(
                TVector<ui8>(127, 0),
                TVector<ui8>(mask.begin() + 1, mask.end())
            );
        }

        {
            auto response = partition.GetChangedBlocks(TBlockRange32(1024, 2047), "cp1", "cp2");

            const auto& mask = response->Record.GetMask();
            UNIT_ASSERT_VALUES_EQUAL(128, mask.size());
            UNIT_ASSERT_VALUES_EQUAL(1, mask[0]);
            AssertEqual(
                TVector<ui8>(127, 0),
                TVector<ui8>(mask.begin() + 1, mask.end())
            );
        }

        {
            auto response = partition.GetChangedBlocks(TBlockRange32(0, 2047), "cp1", "cp2");

            const auto& mask = response->Record.GetMask();
            UNIT_ASSERT_VALUES_EQUAL(256, mask.size());

            UNIT_ASSERT_VALUES_EQUAL(mask[0], 1);
            AssertEqual(
                TVector<ui8>(127, 0),
                TVector<ui8>(mask.begin() + 1, mask.begin() + 128)
            );

            UNIT_ASSERT_VALUES_EQUAL(mask[128], 1);
            AssertEqual(
                TVector<ui8>(127, 0),
                TVector<ui8>(mask.begin() + 129, mask.end())
            );
        }
    }

    Y_UNIT_TEST(ShouldCorrectlyGetChangedBlocksForCheckpointWithoutData)
    {
        auto config = DefaultConfig();

        auto runtime = PrepareTestActorRuntime(config, 2048);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(0,1);
        partition.WriteBlocks(1,1);
        partition.CreateCheckpoint("cp1");

        partition.WriteBlocks(1,2);
        partition.WriteBlocks(2,2);

        partition.DeleteCheckpointData("cp1");
        partition.CreateCheckpoint("cp2");

        auto response = partition.GetChangedBlocks(TBlockRange32(0, 1023), "cp1", "cp2");
        UNIT_ASSERT_VALUES_EQUAL(S_OK, response->GetStatus());

        UNIT_ASSERT_VALUES_EQUAL(128, response->Record.GetMask().size());
        UNIT_ASSERT_VALUES_EQUAL(0b00000110, response->Record.GetMask()[0]);
    }

    Y_UNIT_TEST(ShouldCorrectlyGetChangedBlocksForOverlayDisk)
    {
        TPartitionContent baseContent = {
        /*|      0      |     1     |     2 ... 5    |     6     |      7      |     8     |      9      |*/
            TBlob(1, 1) , TFresh(2) , TBlob(2, 3, 4) ,  TEmpty() , TBlob(1, 4) , TFresh(5) , TBlob(2, 6)
        };

        auto partitionWithRuntime =
            SetupOverlayPartition(TestTabletId, TestTabletId2, baseContent);

        auto& partition = *partitionWithRuntime.Partition;
        partition.WaitReady();

        partition.WriteBlocks(0, 1);
        partition.WriteBlocks(1, 1);
        partition.CreateCheckpoint("cp1");

        partition.WriteBlocks(1, 2);
        partition.WriteBlocks(2, 2);
        partition.CreateCheckpoint("cp2");

        {
            auto response = partition.GetChangedBlocks(TBlockRange32(0, 1023), "cp1", "cp2");

            UNIT_ASSERT_VALUES_EQUAL(128, response->Record.GetMask().size());
            UNIT_ASSERT_VALUES_EQUAL(char(0b00000110), response->Record.GetMask()[0]);
            UNIT_ASSERT_VALUES_EQUAL(0, response->Record.GetMask()[1]);
        }
        {
            auto response = partition.GetChangedBlocks(TBlockRange32(0, 1023), "", "cp2");

            UNIT_ASSERT_VALUES_EQUAL(128, response->Record.GetMask().size());
            UNIT_ASSERT_VALUES_EQUAL(
                char(0b10111111),
                response->Record.GetMask()[0]
            );
            UNIT_ASSERT_VALUES_EQUAL(
                char(0b00000011),
                response->Record.GetMask()[1]
            );
        }

    }

    Y_UNIT_TEST(ShouldCorrectlyGetChangedBlocksWhenRangeIsOutOfBounds)
    {
        auto runtime = PrepareTestActorRuntime(DefaultConfig(), 10);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(1, 1);
        partition.WriteBlocks(2, 1);
        partition.CreateCheckpoint("cp");

        {
            auto response =
                partition.GetChangedBlocks(TBlockRange32(10, 1023), "", "cp");
            UNIT_ASSERT_VALUES_EQUAL(0, response->Record.GetMask().size());
        }

        {
            auto response =
                partition.GetChangedBlocks(TBlockRange32(0, 1023), "", "cp");

            UNIT_ASSERT_VALUES_EQUAL(2, response->Record.GetMask().size());
            UNIT_ASSERT_VALUES_EQUAL(
                char(0b00000110),
                response->Record.GetMask()[0]
            );
            UNIT_ASSERT_VALUES_EQUAL(
                char(0b00000000),
                response->Record.GetMask()[1]
            );
        }
    }

    Y_UNIT_TEST(ShouldConsolidateZeroedBlocksOnCompaction)
    {
        auto config = DefaultConfig();
        config.SetWriteBlobThreshold(1);   // disable FreshBlocks
        config.SetWriteMergedBlobThreshold(Max()); // force Compaction to write mixed blobs

        auto runtime = PrepareTestActorRuntime(config);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(TBlockRange32(0, 500));
        partition.WriteBlocks(TBlockRange32(501, 1000));
        partition.ZeroBlocks(TBlockRange32(501, 1000));

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlobsCount());
            UNIT_ASSERT_VALUES_EQUAL(3, stats.GetMergedBlobsCount());
        }

        partition.Compaction();
        partition.Cleanup();
        partition.CollectGarbage();

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlobsCount());
            UNIT_ASSERT_VALUES_EQUAL(2, stats.GetMergedBlobsCount());
        }

        partition.ZeroBlocks(TBlockRange32(0, 1000));

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlobsCount());
            UNIT_ASSERT_VALUES_EQUAL(3, stats.GetMergedBlobsCount());
        }

        partition.Compaction();
        partition.Cleanup();
        partition.CollectGarbage();

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlobsCount());
            UNIT_ASSERT_VALUES_EQUAL(1, stats.GetMergedBlobsCount());
        }
    }

    Y_UNIT_TEST(ShouldProduceSparseMergedBlobsUponCompaction)
    {
        auto config = DefaultConfig();
        config.SetWriteBlobThreshold(1);   // disable FreshBlocks
        config.SetWriteMergedBlobThreshold(Max());
        config.SetCompactionThreshold(999);

        auto runtime = PrepareTestActorRuntime(config);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(5, 1);
        partition.WriteBlocks(10, 2);
        partition.WriteBlocks(20, 3);
        partition.WriteBlocks(30, 4);
        partition.ZeroBlocks(30);
        partition.ZeroBlocks(40);

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlobsCount());
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(6, stats.GetMergedBlobsCount());
            UNIT_ASSERT_VALUES_EQUAL(4, stats.GetMergedBlocksCount());
        }

        partition.Compaction();
        partition.Cleanup();
        partition.CollectGarbage();

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlobsCount());
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(2, stats.GetMergedBlobsCount());
            UNIT_ASSERT_VALUES_EQUAL(3, stats.GetMergedBlocksCount());
        }

        UNIT_ASSERT_VALUES_EQUAL(
            TStringBuf(),
            GetBlocksContent(partition.ReadBlocks(4))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlocksContent(1),
            GetBlocksContent(partition.ReadBlocks(5))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            TStringBuf(),
            GetBlocksContent(partition.ReadBlocks(6))
        );

        UNIT_ASSERT_VALUES_EQUAL(
            TStringBuf(),
            GetBlocksContent(partition.ReadBlocks(9))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlocksContent(2),
            GetBlocksContent(partition.ReadBlocks(10))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            TStringBuf(),
            GetBlocksContent(partition.ReadBlocks(11))
        );

        UNIT_ASSERT_VALUES_EQUAL(
            TStringBuf(),
            GetBlocksContent(partition.ReadBlocks(19))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlocksContent(3),
            GetBlocksContent(partition.ReadBlocks(20))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            TStringBuf(),
            GetBlocksContent(partition.ReadBlocks(21))
        );

        UNIT_ASSERT_VALUES_EQUAL(
            TStringBuf(),
            GetBlocksContent(partition.ReadBlocks(30))
        );

        UNIT_ASSERT_VALUES_EQUAL(
            TStringBuf(),
            GetBlocksContent(partition.ReadBlocks(40))
        );
    }

    void DoTestIncrementalCompaction(
        NProto::TStorageServiceConfig config,
        bool incrementalCompactionExpected)
    {
        config.SetWriteBlobThreshold(1);   // disable FreshBlocks
        config.SetWriteMergedBlobThreshold(Max());
        config.SetIncrementalCompactionEnabled(true);
        config.SetCompactionThreshold(4);
        config.SetMaxSkippedBlobsDuringCompaction(1);
        config.SetTargetCompactionBytesPerOp(64_KB);

        auto runtime = PrepareTestActorRuntime(config);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        std::unique_ptr<IEventHandle> compactionRequest;
        bool intercept = true;

        runtime->SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvPartitionPrivate::EvCompactionRequest: {
                        if (intercept) {
                            auto request =
                                event->Get<TEvPartitionPrivate::TEvCompactionRequest>();
                            UNIT_ASSERT_VALUES_EQUAL(
                                incrementalCompactionExpected,
                                !request->ForceFullCompaction
                            );

                            compactionRequest.reset(event.Release());
                            intercept = false;

                            return TTestActorRuntime::EEventAction::DROP;
                        }

                        break;
                    }
                }

                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            }
        );

        partition.WriteBlocks(TBlockRange32(0, 1023), 1);
        partition.WriteBlocks(TBlockRange32(0, 4), 2);
        partition.WriteBlocks(TBlockRange32(10, 14), 3);
        partition.WriteBlocks(TBlockRange32(20, 25), 4);

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlobsCount());
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(4, stats.GetMergedBlobsCount());
            UNIT_ASSERT_VALUES_EQUAL(1040, stats.GetMergedBlocksCount());
        }

        runtime->DispatchEvents({}, TDuration::Seconds(1));
        UNIT_ASSERT(compactionRequest);
        runtime->Send(compactionRequest.release());
        runtime->DispatchEvents({}, TDuration::Seconds(1));
        partition.Cleanup();
        partition.CollectGarbage();

        {
            ui32 blobs = incrementalCompactionExpected ? 2 : 1;
            ui32 blocks = incrementalCompactionExpected ? 1040 : 1024;

            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlobsCount());
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(blobs, stats.GetMergedBlobsCount());
            UNIT_ASSERT_VALUES_EQUAL(blocks, stats.GetMergedBlocksCount());
        }

        for (ui32 i = 0; i <= 4; ++i) {
            UNIT_ASSERT_VALUES_EQUAL(
                GetBlocksContent(2),
                GetBlocksContent(partition.ReadBlocks(i))
            );
        }

        for (ui32 i = 5; i < 10; ++i) {
            UNIT_ASSERT_VALUES_EQUAL(
                GetBlocksContent(1),
                GetBlocksContent(partition.ReadBlocks(i))
            );
        }

        for (ui32 i = 10; i <= 14; ++i) {
            UNIT_ASSERT_VALUES_EQUAL(
                GetBlocksContent(3),
                GetBlocksContent(partition.ReadBlocks(i))
            );
        }

        for (ui32 i = 15; i < 20; ++i) {
            UNIT_ASSERT_VALUES_EQUAL(
                GetBlocksContent(1),
                GetBlocksContent(partition.ReadBlocks(i))
            );
        }

        for (ui32 i = 20; i <= 25; ++i) {
            UNIT_ASSERT_VALUES_EQUAL(
                GetBlocksContent(4),
                GetBlocksContent(partition.ReadBlocks(i))
            );
        }

        for (ui32 i = 26; i < 1023; ++i) {
            UNIT_ASSERT_VALUES_EQUAL(
                GetBlocksContent(1),
                GetBlocksContent(partition.ReadBlocks(i))
            );
        }
    }

    Y_UNIT_TEST(ShouldCompactIncrementally)
    {
        auto config = DefaultConfig();
        config.SetCompactionGarbageThreshold(20);
        DoTestIncrementalCompaction(std::move(config), true);
    }

    Y_UNIT_TEST(ShouldNotCompactIncrementallyIfDiskGarbageLevelIsTooHigh)
    {
        auto config = DefaultConfig();
        config.SetCompactionGarbageThreshold(1);
        DoTestIncrementalCompaction(std::move(config), false);
    }

    Y_UNIT_TEST(ShouldNotEraseBlocksForSkippedBlobsFromIndexDuringIncrementalCompaction)
    {
        auto config = DefaultConfig();
        config.SetWriteBlobThreshold(1_GB);   // everything goes to fresh
        config.SetWriteMergedBlobThreshold(Max());
        config.SetIncrementalCompactionEnabled(true);
        config.SetCompactionThreshold(999);
        config.SetMaxSkippedBlobsDuringCompaction(1);
        config.SetTargetCompactionBytesPerOp(1);

        auto runtime = PrepareTestActorRuntime(config);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        // blob 1 needs to eventually have more live blocks than blob 2 in order
        // not to be compacted
        partition.WriteBlocks(TBlockRange32(2, 22), 1);
        partition.Flush();
        partition.WriteBlocks(TBlockRange32(1, 10), 2);
        partition.Flush();

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(2, stats.GetMixedBlobsCount());
            UNIT_ASSERT_VALUES_EQUAL(31, stats.GetMixedBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMergedBlobsCount());
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMergedBlocksCount());
        }

        partition.Compaction();
        partition.Cleanup();
        partition.CollectGarbage();

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(1, stats.GetMixedBlobsCount());
            UNIT_ASSERT_VALUES_EQUAL(21, stats.GetMixedBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(1, stats.GetMergedBlobsCount());
            UNIT_ASSERT_VALUES_EQUAL(10, stats.GetMergedBlocksCount());
        }

        for (ui32 i = 11; i <= 22; ++i) {
            UNIT_ASSERT_VALUES_EQUAL(
                GetBlocksContent(1),
                GetBlocksContent(partition.ReadBlocks(i))
            );
        }

        for (ui32 i = 1; i <= 10; ++i) {
            UNIT_ASSERT_VALUES_EQUAL(
                GetBlocksContent(2),
                GetBlocksContent(partition.ReadBlocks(i))
            );
        }
    }

    Y_UNIT_TEST(ShouldProperlyProcessZeroesDuringIncrementalCompaction)
    {
        auto config = DefaultConfig();
        config.SetWriteBlobThreshold(1);    // disabling fresh
        config.SetWriteMergedBlobThreshold(1);
        config.SetIncrementalCompactionEnabled(true);
        config.SetCompactionThreshold(999);
        config.SetMaxSkippedBlobsDuringCompaction(1);
        config.SetTargetCompactionBytesPerOp(1);

        auto runtime = PrepareTestActorRuntime(config);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        // writing some data
        partition.WriteBlocks(TBlockRange32(0, 10), 1);
        // zeroing it
        partition.ZeroBlocks(TBlockRange32(0, 10));
        // writing some more data above
        partition.WriteBlocks(TBlockRange32(1, 12), 2);

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlobsCount());
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(3, stats.GetMergedBlobsCount());
            UNIT_ASSERT_VALUES_EQUAL(23, stats.GetMergedBlocksCount());
        }

        partition.Compaction();
        partition.Cleanup();
        partition.CollectGarbage();

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlobsCount());
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(2, stats.GetMergedBlobsCount());
            UNIT_ASSERT_VALUES_EQUAL(12, stats.GetMergedBlocksCount());
        }

        UNIT_ASSERT_VALUES_EQUAL(
            TStringBuf(),
            GetBlocksContent(partition.ReadBlocks(0))
        );

        for (ui32 i = 1; i <= 12; ++i) {
            UNIT_ASSERT_VALUES_EQUAL(
                GetBlocksContent(2),
                GetBlocksContent(partition.ReadBlocks(i))
            );
        }
    }

    Y_UNIT_TEST(ShouldRejectCompactionRequestsIfDataChannelsAreAlmostFull)
    {
        // smoke test: tests that the compaction mechanism checks partition state
        // detailed tests are located in part_state_ut.cpp
        auto runtime = PrepareTestActorRuntime();

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        runtime->SetObserverFunc(
            StorageStateChanger(NKikimrBlobStorage::StatusDiskSpaceLightYellowMove));
        partition.WriteBlocks(TBlockRange32(0, 1023));

        {
            auto response = partition.Compaction();
            UNIT_ASSERT_VALUES_EQUAL(S_OK, response->GetError().GetCode());
        }

        const auto badFlags = {
            NKikimrBlobStorage::StatusDiskSpaceOrange,
            NKikimrBlobStorage::StatusDiskSpaceRed,
        };

        for (const auto flag: badFlags) {
            partition.RebootTablet();

            runtime->SetObserverFunc(StorageStateChanger(flag));
            partition.WriteBlocks(TBlockRange32(0, 1023));

            auto request = partition.CreateCompactionRequest();
            partition.SendToPipe(std::move(request));

            auto response = partition.RecvResponse<TEvPartitionPrivate::TEvCompactionResponse>();
            UNIT_ASSERT_VALUES_EQUAL(E_BS_OUT_OF_SPACE, response->GetError().GetCode());
        }
    }

    Y_UNIT_TEST(ShouldReassignNonwritableTabletChannels)
    {
        const auto channelCount = 6;
        const auto groupCount = channelCount - DataChannelOffset;
        const auto reassignTimeout = TDuration::Minutes(1);

        TTestEnv env(0, 1, channelCount, groupCount);
        auto& runtime = env.GetRuntime();

        NMonitoring::TDynamicCountersPtr counters
            = new NMonitoring::TDynamicCounters();
        InitCriticalEventsCounter(counters);
        auto reassignCounter =
            counters->GetCounter("AppCriticalEvents/ReassignTablet", true);

        NProto::TStorageServiceConfig config;
        config.SetReassignRequestRetryTimeout(reassignTimeout.MilliSeconds());
        const auto tabletId = InitTestActorRuntime(
            env,
            runtime,
            channelCount,
            channelCount,
            config);

        TPartitionClient partition(runtime, 0, tabletId);
        partition.WaitReady();

        ui64 reassignedTabletId = 0;
        TVector<ui32> channels;
        ui32 ssflags = ui32(
            NKikimrBlobStorage::StatusDiskSpaceYellowStop |
            NKikimrBlobStorage::StatusDiskSpaceLightYellowMove);

        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvHiveProxy::EvReassignTabletRequest: {
                        auto* msg = event->Get<TEvHiveProxy::TEvReassignTabletRequest>();
                        reassignedTabletId = msg->TabletId;
                        channels = msg->Channels;

                        break;
                    }
                }

                return StorageStateChanger(ssflags, env.GetGroupIds()[1])(
                    runtime,
                    event
                );
            }
        );

        // first request does not trigger a reassign event - data is written to
        // the first group
        partition.WriteBlocks(TBlockRange32(0, 1023));
        // second request succeeds but triggers a reassign event - yellow flag
        // is received in the response for this request
        partition.WriteBlocks(TBlockRange32(0, 1023));

        UNIT_ASSERT_VALUES_EQUAL(1, reassignCounter->Val());

        UNIT_ASSERT_VALUES_EQUAL(tabletId, reassignedTabletId);
        UNIT_ASSERT_VALUES_EQUAL(1, channels.size());
        UNIT_ASSERT_VALUES_EQUAL(4, channels.front());

        reassignedTabletId = 0;
        channels.clear();

        {
            // third request doesn't trigger a reassign event because we can
            // still write (some data channels are not yellow yet)
            auto request =
                partition.CreateWriteBlocksRequest(TBlockRange32(0, 1023));
            partition.SendToPipe(std::move(request));

            auto response =
                partition.RecvResponse<TEvService::TEvWriteBlocksResponse>();
            UNIT_ASSERT_VALUES_EQUAL(
                S_OK,
                response->GetError().GetCode()
            );
        }

        // checking that a reassign request has not been sent
        UNIT_ASSERT_VALUES_EQUAL(1, reassignCounter->Val());

        UNIT_ASSERT_VALUES_EQUAL(0, reassignedTabletId);
        UNIT_ASSERT_VALUES_EQUAL(0, channels.size());

        reassignedTabletId = 0;
        channels.clear();

        {
            // informing partition tablet that the first group is yellow
            auto request =
                std::make_unique<TEvTablet::TEvCheckBlobstorageStatusResult>(
                    TVector<ui32>({env.GetGroupIds()[0], env.GetGroupIds()[1]}),
                    TVector<ui32>({env.GetGroupIds()[0], env.GetGroupIds()[1]})
                );
            partition.SendToPipe(std::move(request));
        }

        {
            auto request =
                partition.CreateWriteBlocksRequest(TBlockRange32(0, 1023));
            partition.SendToPipe(std::move(request));

            auto response =
                partition.RecvResponse<TEvService::TEvWriteBlocksResponse>();
            UNIT_ASSERT_VALUES_EQUAL(
                E_BS_OUT_OF_SPACE,
                response->GetError().GetCode()
            );
        }

        // this time no reassign request should have been sent because of the
        // ReassignRequestSentTs check in partition actor
        UNIT_ASSERT_VALUES_EQUAL(1, reassignCounter->Val());

        UNIT_ASSERT_VALUES_EQUAL(0, reassignedTabletId);
        UNIT_ASSERT_VALUES_EQUAL(0, channels.size());

        runtime.AdvanceCurrentTime(reassignTimeout);

        {
            auto request =
                partition.CreateWriteBlocksRequest(TBlockRange32(0, 1023));
            partition.SendToPipe(std::move(request));

            auto response =
                partition.RecvResponse<TEvService::TEvWriteBlocksResponse>();
            UNIT_ASSERT_VALUES_EQUAL(
                E_BS_OUT_OF_SPACE,
                response->GetError().GetCode()
            );
        }

        // ReassignRequestRetryTimeout has passed - another reassign request
        // should've been sent
        UNIT_ASSERT_VALUES_EQUAL(2, reassignCounter->Val());

        UNIT_ASSERT_VALUES_EQUAL(tabletId, reassignedTabletId);
        UNIT_ASSERT_VALUES_EQUAL(5, channels.size());
        for (ui32 i = 0; i < channels.size(); ++i) {
            UNIT_ASSERT_VALUES_EQUAL(i, channels[i]);
        }

        partition.RebootTablet();
        ssflags = {};

        // no yellow channels => write request should succeed
        partition.WriteBlocks(TBlockRange32(0, 1023));

        partition.RebootTablet();

        {
            // informing partition tablet that the first group is yellow
            auto request =
                std::make_unique<TEvTablet::TEvCheckBlobstorageStatusResult>(
                    TVector<ui32>({env.GetGroupIds()[0]}),
                    TVector<ui32>({env.GetGroupIds()[0]})
                );
            partition.SendToPipe(std::move(request));
        }

        runtime.DispatchEvents({});

        {
            auto request =
                partition.CreateWriteBlocksRequest(TBlockRange32(0, 1023));
            partition.SendToPipe(std::move(request));

            auto response =
                partition.RecvResponse<TEvService::TEvWriteBlocksResponse>();
            UNIT_ASSERT_VALUES_EQUAL(
                E_BS_OUT_OF_SPACE,
                response->GetError().GetCode()
            );
        }

        // checking that a reassign request has been sent
        UNIT_ASSERT_VALUES_EQUAL(3, reassignCounter->Val());

        UNIT_ASSERT_VALUES_EQUAL(tabletId, reassignedTabletId);
        UNIT_ASSERT_VALUES_EQUAL(4, channels.size());
        UNIT_ASSERT_VALUES_EQUAL(0, channels[0]);
        UNIT_ASSERT_VALUES_EQUAL(1, channels[1]);
        UNIT_ASSERT_VALUES_EQUAL(2, channels[2]);
        UNIT_ASSERT_VALUES_EQUAL(3, channels[3]);
    }

    Y_UNIT_TEST(ShouldReassignNonwritableTabletChannelsAgainAfterErrorFromHiveProxy)
    {
        auto runtime = PrepareTestActorRuntime();

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        ui64 reassignedTabletId = 0;
        TVector<ui32> channels;
        ui32 ssflags = ui32(
            NKikimrBlobStorage::StatusDiskSpaceYellowStop |
            NKikimrBlobStorage::StatusDiskSpaceLightYellowMove);

        runtime->SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvHiveProxy::EvReassignTabletRequest: {
                        auto* msg = event->Get<TEvHiveProxy::TEvReassignTabletRequest>();
                        reassignedTabletId = msg->TabletId;
                        channels = msg->Channels;
                        auto response =
                            std::make_unique<TEvHiveProxy::TEvReassignTabletResponse>(
                                MakeError(E_REJECTED, "error")
                            );
                        runtime.Send(new IEventHandle(
                            event->Sender,
                            event->Recipient,
                            response.release(),
                            0, // flags
                            0
                        ), 0);

                        return TTestActorRuntime::EEventAction::DROP;
                    }
                }

                return StorageStateChanger(ssflags)(runtime, event);
            }
        );

        // first request is successful since we don't know that our channels
        // are yellow yet
        partition.WriteBlocks(TBlockRange32(0, 1023));

        // but upon EvPutResult we should find out that our channels are yellow
        // and should send a reassign request
        UNIT_ASSERT_VALUES_EQUAL(TestTabletId, reassignedTabletId);
        UNIT_ASSERT_VALUES_EQUAL(1, channels.size());
        UNIT_ASSERT_VALUES_EQUAL(3, channels.front());

        reassignedTabletId = 0;
        channels.clear();

        for (ui32 i = 0; i < 3; ++i) {
            {
                // other requests should fail
                auto request =
                    partition.CreateWriteBlocksRequest(TBlockRange32(0, 1023));
                partition.SendToPipe(std::move(request));

                auto response =
                    partition.RecvResponse<TEvService::TEvWriteBlocksResponse>();
                UNIT_ASSERT_VALUES_EQUAL(
                    E_BS_OUT_OF_SPACE,
                    response->GetError().GetCode()
                );
            }

            // checking that a reassign request has been sent
            UNIT_ASSERT_VALUES_EQUAL(TestTabletId, reassignedTabletId);
            UNIT_ASSERT_VALUES_EQUAL(1, channels.size());
            UNIT_ASSERT_VALUES_EQUAL(3, channels.front());

            reassignedTabletId = 0;
            channels.clear();
        }
    }

    Y_UNIT_TEST(ShouldSendBackpressureReportsUponChannelColorChange)
    {
        auto runtime = PrepareTestActorRuntime();

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        TBackpressureReport report;

        runtime->SetObserverFunc(
            [&report] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvPartition::EvBackpressureReport: {
                        report = *event->Get<TEvPartition::TEvBackpressureReport>();
                        break;
                    }
                }

                return StorageStateChanger(
                    NKikimrBlobStorage::StatusDiskSpaceLightYellowMove |
                    NKikimrBlobStorage::StatusDiskSpaceYellowStop)(runtime, event);
            }
        );

        partition.WriteBlocks(TBlockRange32(0, 1023));

        UNIT_ASSERT(report.DiskSpaceScore > 0);
    }

    Y_UNIT_TEST(ShouldSendBackpressureReportsRegularly)
    {
        const auto blockCount = 1000;
        auto runtime = PrepareTestActorRuntime(DefaultConfig(), blockCount);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        TBackpressureReport report;
        bool reportUpdated = false;

        runtime->SetObserverFunc(
            [&report, &reportUpdated] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvPartition::EvBackpressureReport: {
                        report = *event->Get<TEvPartition::TEvBackpressureReport>();
                        reportUpdated = true;
                        break;
                    }
                }

                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            }
        );

        for (int i = 0; i < 200; ++i) {
            partition.WriteBlocks(i, i);
        }

        // Manually sending TEvSendBackpressureReport here
        // In real scenarios TPartitionActor will schedule this event
        // upon executor activation
        reportUpdated = false;
        partition.SendToPipe(
            std::make_unique<TEvPartitionPrivate::TEvSendBackpressureReport>()
        );

        runtime->DispatchEvents({}, TDuration::Seconds(5));

        UNIT_ASSERT_VALUES_EQUAL(true, reportUpdated);
        UNIT_ASSERT_DOUBLES_EQUAL(3.5, report.FreshIndexScore, 1e-5);
    }

    Y_UNIT_TEST(ShouldRejectRequestsInProgressOnTabletDeath)
    {
        auto runtime = PrepareTestActorRuntime();

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        runtime->SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                if (event->GetTypeRewrite() == TEvPartitionPrivate::EvWriteBlobResponse) {
                    return TTestActorRuntime::EEventAction::DROP;
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        partition.SendWriteBlocksRequest(TBlockRange32(0, 1023));

        // wait for background operations completion
        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        // kill tablet
        partition.RebootTablet();

        auto response = partition.RecvWriteBlocksResponse();
        UNIT_ASSERT_C(response->GetStatus() == E_REJECTED, response->GetErrorReason());
    }

    Y_UNIT_TEST(ShouldReportNumberOfNonEmptyRangesInVolumeStats)
    {
        auto config = DefaultConfig();

        auto runtime = PrepareTestActorRuntime(config, 4096 * MaxBlocksCount);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        for (ui32 g = 0; g < 4; ++g) {
            partition.WriteBlocks(g * MaxBlocksCount * MaxBlocksCount, 1);
        }

        partition.Flush();

        auto response = partition.StatPartition();
        const auto& stats = response->Record.GetStats();
        UNIT_ASSERT_VALUES_EQUAL(4, stats.GetNonEmptyRangeCount());
    }

    Y_UNIT_TEST(ShouldHandleDescribeBlocksRequestWithInvalidCheckpointId)
    {
        auto runtime = PrepareTestActorRuntime();
        TPartitionClient partition(*runtime);
        partition.WaitReady();

        const TString validCheckpoint = "0";
        partition.CreateCheckpoint(validCheckpoint);

        const auto range = TBlockRange32::WithLength(0, 1);
        partition.DescribeBlocks(range, validCheckpoint);

        {
            const TString invalidCheckpoint = "1";
            auto request =
                partition.CreateDescribeBlocksRequest(range, invalidCheckpoint);
            partition.SendToPipe(std::move(request));
            auto response =
                partition.RecvResponse<TEvVolume::TEvDescribeBlocksResponse>();
            UNIT_ASSERT(response->GetStatus() == E_NOT_FOUND);
        }
    }

    Y_UNIT_TEST(ShouldForbidDescribeBlocksWithEmptyRange)
    {
        auto runtime = PrepareTestActorRuntime();
        TPartitionClient partition(*runtime);
        partition.WaitReady();

        {
            auto request = partition.CreateDescribeBlocksRequest(0, 0);
            partition.SendToPipe(std::move(request));
            auto response =
                partition.RecvResponse<TEvVolume::TEvDescribeBlocksResponse>();
            UNIT_ASSERT(response->GetStatus() == E_ARGUMENT);
        }
    }

    Y_UNIT_TEST(ShouldHandleDescribeBlocksRequestWithOutOfBoundsRange)
    {
        auto runtime = PrepareTestActorRuntime();
        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(TBlockRange32(0, 10), char(1));

        {
            const TBlockRange32 range(Max<ui32>() - 1, Max<ui32>());
            auto request = partition.CreateDescribeBlocksRequest(range);
            partition.SendToPipe(std::move(request));
            auto response =
                partition.RecvResponse<TEvVolume::TEvDescribeBlocksResponse>();
            UNIT_ASSERT(response->GetStatus() == S_OK);
            UNIT_ASSERT(response->Record.FreshBlockRangesSize() == 0);
            UNIT_ASSERT(response->Record.BlobPiecesSize() == 0);
        }
    }

    Y_UNIT_TEST(ShouldHandleDescribeBlocksRequestWhenBlocksAreFresh)
    {
        auto runtime = PrepareTestActorRuntime();
        TPartitionClient partition(*runtime);
        partition.WaitReady();

        const auto range = TBlockRange32::WithLength(1, 11);
        partition.WriteBlocks(range, char(1));

        const TString checkpoint = "0";
        partition.CreateCheckpoint(checkpoint);

        {
            auto response = partition.DescribeBlocks(range, checkpoint);
            TString actualContent;
            TVector<TBlockRange32> actualRanges;

            auto extractContent = [&]() {
                const auto& message = response->Record;
                for (size_t i = 0; i < message.FreshBlockRangesSize(); ++i) {
                    const auto& freshRange = message.GetFreshBlockRanges(i);
                    actualContent += freshRange.GetBlocksContent();
                    actualRanges.push_back(
                        TBlockRange32::WithLength(
                            freshRange.GetStartIndex(),
                            freshRange.GetBlocksCount()));
                 }
            };

            extractContent();
            TString expectedContent = GetBlocksContent(char(1), range.Size());
            UNIT_ASSERT_VALUES_EQUAL(expectedContent, actualContent);
            CheckRangesArePartition(actualRanges, range);

            response = partition.DescribeBlocks(0, Max<ui32>(), checkpoint);
            actualContent = {};
            actualRanges = {};

            extractContent();
            expectedContent = GetBlocksContent(char(1), range.Size());
            UNIT_ASSERT_VALUES_EQUAL(expectedContent, actualContent);
            CheckRangesArePartition(actualRanges, range);
        }
    }

    Y_UNIT_TEST(ShouldHandleDescribeBlocksRequestWithOneBlob)
    {
        auto runtime = PrepareTestActorRuntime();
        TPartitionClient partition(*runtime);
        partition.WaitReady();

        const auto range1 = TBlockRange32::WithLength(11, 11);
        partition.WriteBlocks(range1, char(1));
        const auto range2 = TBlockRange32::WithLength(33, 22);
        partition.WriteBlocks(range2, char(1));
        partition.Flush();

        {
            const auto response = partition.DescribeBlocks(TBlockRange32(0, 100));
            const auto& message = response->Record;
            // Expect that all data is contained in one blob.
            UNIT_ASSERT_VALUES_EQUAL(1, message.BlobPiecesSize());

            const auto& blobPiece = message.GetBlobPieces(0);
            UNIT_ASSERT_VALUES_EQUAL(2, blobPiece.RangesSize());
            const auto& r1 = blobPiece.GetRanges(0);
            const auto& r2 = blobPiece.GetRanges(1);

            UNIT_ASSERT_VALUES_EQUAL(0, r1.GetBlobOffset());
            UNIT_ASSERT_VALUES_EQUAL(range1.Start, r1.GetBlockIndex());
            UNIT_ASSERT_VALUES_EQUAL(range1.Size(), r1.GetBlocksCount());

            UNIT_ASSERT_VALUES_EQUAL(range1.Size(), r2.GetBlobOffset());
            UNIT_ASSERT_VALUES_EQUAL(range2.Start, r2.GetBlockIndex());
            UNIT_ASSERT_VALUES_EQUAL(range2.Size(), r2.GetBlocksCount());

            TVector<ui16> blobOffsets;
            for (size_t i = 0; i < r1.GetBlocksCount(); ++i) {
                blobOffsets.push_back(r1.GetBlobOffset() + i);
            }
            for (size_t i = 0; i < r2.GetBlocksCount(); ++i) {
                blobOffsets.push_back(r2.GetBlobOffset() + i);
            }

            {
                TVector<TString> blocks;
                auto sglist = ResizeBlocks(
                    blocks,
                    range1.Size() + range2.Size(),
                    TString(DefaultBlockSize, char(0)));

                const auto blobId =
                    LogoBlobIDFromLogoBlobID(blobPiece.GetBlobId());
                const auto group = blobPiece.GetBSGroupId();
                const auto response =
                    partition.ReadBlob(blobId, group, blobOffsets, sglist);

                for (size_t i = 0; i < sglist.size(); ++i) {
                    const auto block = sglist[i].AsStringBuf();
                    UNIT_ASSERT_VALUES_EQUAL_C(
                        GetBlockContent(
                            char(1)),
                            block,
                            "during iteration #" << i);
                }
            }
        }
    }

    Y_UNIT_TEST(ShouldRespondToDescribeBlocksRequestWithDenseBlobs)
    {
        auto runtime = PrepareTestActorRuntime();
        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(0, char(1));
        partition.WriteBlocks(1, char(1));
        partition.WriteBlocks(2, char(1));
        partition.Flush();
        partition.WriteBlocks(3, char(2));
        partition.WriteBlocks(4, char(2));
        partition.WriteBlocks(5, char(2));
        partition.Flush();

        {
            const auto response = partition.DescribeBlocks(TBlockRange32(1, 4));
            const auto& message = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(2, message.BlobPiecesSize());

            const auto& blobPiece1 = message.GetBlobPieces(0);
            UNIT_ASSERT_VALUES_EQUAL(1, blobPiece1.RangesSize());
            const auto& r1_2 = blobPiece1.GetRanges(0);
            UNIT_ASSERT_VALUES_EQUAL(1, r1_2.GetBlobOffset());
            UNIT_ASSERT_VALUES_EQUAL(1, r1_2.GetBlockIndex());
            UNIT_ASSERT_VALUES_EQUAL(2, r1_2.GetBlocksCount());
            const auto blobId1 =
                LogoBlobIDFromLogoBlobID(blobPiece1.GetBlobId());
            const auto group1 = blobPiece1.GetBSGroupId();

            const auto& blobPiece2 = message.GetBlobPieces(1);
            UNIT_ASSERT_VALUES_EQUAL(1, blobPiece2.RangesSize());
            const auto& r3_5 = blobPiece2.GetRanges(0);
            UNIT_ASSERT_VALUES_EQUAL(0, r3_5.GetBlobOffset());
            UNIT_ASSERT_VALUES_EQUAL(3, r3_5.GetBlockIndex());
            UNIT_ASSERT_VALUES_EQUAL(2, r3_5.GetBlocksCount());
            const auto blobId2 =
                LogoBlobIDFromLogoBlobID(blobPiece2.GetBlobId());
            const auto group2 = blobPiece2.GetBSGroupId();

            for (ui16 i = 0; i < 2; ++i) {
                const TVector<ui16> offsets = {i};
                {
                    TVector<TString> blocks;
                    auto sglist = ResizeBlocks(blocks, 1, TString(DefaultBlockSize, char(0)));
                    const auto response =
                        partition.ReadBlob(blobId1, group1, offsets, sglist);
                    UNIT_ASSERT_VALUES_EQUAL(
                        GetBlockContent(char(1)),
                        sglist[0].AsStringBuf()
                    );
                }

                {
                    TVector<TString> blocks;
                    auto sglist = ResizeBlocks(blocks, 1, TString(DefaultBlockSize, char(0)));
                    const auto response =
                        partition.ReadBlob(blobId2, group2, offsets, sglist);
                    UNIT_ASSERT_VALUES_EQUAL(
                        GetBlockContent(char(2)),
                        sglist[0].AsStringBuf()
                    );
                }
            }
        }
    }

    Y_UNIT_TEST(ShouldRespondToDescribeBlocksRequestWithSparseBlobs)
    {
        auto runtime = PrepareTestActorRuntime();
        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(0, char(1));
        partition.WriteBlocks(2, char(1));
        partition.Flush();
        partition.WriteBlocks(4, char(2));
        partition.WriteBlocks(6, char(2));
        partition.Flush();

        {
            const auto response = partition.DescribeBlocks(TBlockRange32(0, 6));
            const auto& message = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(2, message.BlobPiecesSize());

            const auto& blobPiece1 = message.GetBlobPieces(0);
            UNIT_ASSERT_VALUES_EQUAL(2, blobPiece1.RangesSize());
            const auto& r0 = blobPiece1.GetRanges(0);
            UNIT_ASSERT_VALUES_EQUAL(0, r0.GetBlobOffset());
            UNIT_ASSERT_VALUES_EQUAL(0, r0.GetBlockIndex());
            UNIT_ASSERT_VALUES_EQUAL(1, r0.GetBlocksCount());
            const auto& r1 = blobPiece1.GetRanges(1);
            UNIT_ASSERT_VALUES_EQUAL(1, r1.GetBlobOffset());
            UNIT_ASSERT_VALUES_EQUAL(2, r1.GetBlockIndex());
            UNIT_ASSERT_VALUES_EQUAL(1, r1.GetBlocksCount());
            const auto blobId1 =
                LogoBlobIDFromLogoBlobID(blobPiece1.GetBlobId());
            const auto group1 = blobPiece1.GetBSGroupId();

            const auto& blobPiece2 = message.GetBlobPieces(1);
            UNIT_ASSERT_VALUES_EQUAL(2, blobPiece2.RangesSize());
            const auto& r4 = blobPiece2.GetRanges(0);
            UNIT_ASSERT_VALUES_EQUAL(0, r4.GetBlobOffset());
            UNIT_ASSERT_VALUES_EQUAL(4, r4.GetBlockIndex());
            UNIT_ASSERT_VALUES_EQUAL(1, r4.GetBlocksCount());
            const auto& r6 = blobPiece2.GetRanges(1);
            UNIT_ASSERT_VALUES_EQUAL(1, r6.GetBlobOffset());
            UNIT_ASSERT_VALUES_EQUAL(6, r6.GetBlockIndex());
            UNIT_ASSERT_VALUES_EQUAL(1, r6.GetBlocksCount());
            const auto blobId2 =
                LogoBlobIDFromLogoBlobID(blobPiece2.GetBlobId());
            const auto group2 = blobPiece2.GetBSGroupId();

            for (ui16 i = 0; i < 1; ++i) {
                const TVector<ui16> offsets = {i};
                {
                    TVector<TString> blocks;
                    auto sglist = ResizeBlocks(blocks, 1, TString(DefaultBlockSize, char(0)));
                    const auto response =
                        partition.ReadBlob(blobId1, group1, offsets, sglist);
                    UNIT_ASSERT_VALUES_EQUAL(
                        GetBlockContent(char(1)),
                        sglist[0].AsStringBuf()
                    );
                }

                {
                    TVector<TString> blocks;
                    auto sglist = ResizeBlocks(blocks, 1, TString(DefaultBlockSize, char(0)));
                    const auto response =
                        partition.ReadBlob(blobId2, group2, offsets, sglist);
                    UNIT_ASSERT_VALUES_EQUAL(
                        GetBlockContent(char(2)),
                        sglist[0].AsStringBuf()
                    );
                }
            }
        }
    }

    Y_UNIT_TEST(ShouldReturnExactlyOneVersionOfEachBlockInDescribeBlocksResponse) {
        auto runtime = PrepareTestActorRuntime();
        TPartitionClient partition(*runtime);
        partition.WaitReady();

        // Without fresh blocks.
        partition.WriteBlocks(0, char(1));
        partition.Flush();
        partition.CreateCheckpoint("checkpoint1");
        partition.WriteBlocks(0, char(2));
        partition.Flush();
        {
            const auto response = partition.DescribeBlocks(TBlockRange32(0, 0));
            UNIT_ASSERT_VALUES_EQUAL(0, response->Record.FreshBlockRangesSize());

            UNIT_ASSERT_VALUES_EQUAL(1, response->Record.BlobPiecesSize());
            const auto& blobPiece = response->Record.GetBlobPieces(0);
            UNIT_ASSERT_VALUES_EQUAL(1, blobPiece.RangesSize());
            UNIT_ASSERT_VALUES_EQUAL(0, blobPiece.GetRanges(0).GetBlobOffset());
            UNIT_ASSERT_VALUES_EQUAL(0, blobPiece.GetRanges(0).GetBlockIndex());

            const auto blobId = LogoBlobIDFromLogoBlobID(blobPiece.GetBlobId());
            const auto group = blobPiece.GetBSGroupId();
            {
                TVector<TString> blocks;
                auto sglist = ResizeBlocks(blocks, 1, TString(DefaultBlockSize, char(0)));
                const auto response = partition.ReadBlob(blobId, group, TVector<ui16>{0}, sglist);
                UNIT_ASSERT_VALUES_EQUAL(
                    GetBlockContent(char(2)),
                    sglist[0].AsStringBuf()
                );
            }
        }

        // With fresh blocks.
        partition.WriteBlocks(0, char(3));
        partition.CreateCheckpoint("checkpoint2");
        partition.WriteBlocks(0, char(4));
        {
            const auto response = partition.DescribeBlocks(TBlockRange32(0, 0));
            UNIT_ASSERT_VALUES_EQUAL(0, response->Record.BlobPiecesSize());

            UNIT_ASSERT_VALUES_EQUAL(1, response->Record.FreshBlockRangesSize());
            const auto& freshBlockRange = response->Record.GetFreshBlockRanges(0);
            UNIT_ASSERT_VALUES_EQUAL(1, freshBlockRange.GetBlocksCount());

            UNIT_ASSERT_VALUES_EQUAL(
                GetBlockContent(char(4)),
                freshBlockRange.GetBlocksContent()
            );
        }
    }

    Y_UNIT_TEST(ShouldCorrectlyCalculateUsedBlocksCount)
    {
        constexpr ui32 blockCount = 1024 * 1024;
        auto config = DefaultConfig();
        config.SetLogicalUsedBlocksCalculationEnabled(true);

        auto runtime = PrepareTestActorRuntime(config, blockCount);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(TBlockRange32(0, 1024 * 10), 1);
        partition.WriteBlocks(TBlockRange32(1024 * 5, 1024 * 11), 1);

        const auto step = 16;
        for (ui32 i = 1024 * 10; i < 1024 * 12; i += step) {
            partition.WriteBlocks(TBlockRange32(i, i + step - 1), 1);
        }

        for (ui32 i = 1024 * 20; i < 1024 * 21; i += step) {
            partition.WriteBlocks(TBlockRange32(i, i + step), 1);
        }

        partition.WriteBlocks(TBlockRange32(1001111, 1001210), 1);

        partition.ZeroBlocks(TBlockRange32(1024, 3023));
        partition.ZeroBlocks(TBlockRange32(5024, 5033));

        const auto expected = 1024 * 12 + 1024 + 1 + 100 - 2000 - 10;

        /*
        partition.WriteBlocks(TBlockRange32(5024, 5043), 1);
        partition.ZeroBlocks(TBlockRange32(5024, 5033));
        const auto expected = 10;
        */

        auto response = partition.StatPartition();
        auto stats = response->Record.GetStats();
        UNIT_ASSERT_VALUES_EQUAL(expected, stats.GetUsedBlocksCount());
        UNIT_ASSERT_VALUES_EQUAL(expected, stats.GetLogicalUsedBlocksCount());

        partition.RebootTablet();

        response = partition.StatPartition();
        stats = response->Record.GetStats();
        UNIT_ASSERT_VALUES_EQUAL(expected, stats.GetUsedBlocksCount());
        UNIT_ASSERT_VALUES_EQUAL(expected, stats.GetLogicalUsedBlocksCount());

        ui32 completionStatus = -1;
        runtime->SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvPartitionPrivate::EvMetadataRebuildCompleted: {
                        using TEv =
                            TEvPartitionPrivate::TEvMetadataRebuildCompleted;
                        auto* msg = event->Get<TEv>();
                        completionStatus = msg->GetStatus();
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            }
        );

        const auto rangesPerBatch = 100;
        partition.RebuildMetadata(NProto::ERebuildMetadataType::USED_BLOCKS, rangesPerBatch);

        UNIT_ASSERT_VALUES_EQUAL(S_OK, completionStatus);

        response = partition.StatPartition();
        stats = response->Record.GetStats();
        UNIT_ASSERT_VALUES_EQUAL(expected, stats.GetUsedBlocksCount());
        UNIT_ASSERT_VALUES_EQUAL(expected, stats.GetLogicalUsedBlocksCount());

        partition.RebootTablet();

        response = partition.StatPartition();
        stats = response->Record.GetStats();
        UNIT_ASSERT_VALUES_EQUAL(expected, stats.GetUsedBlocksCount());
        UNIT_ASSERT_VALUES_EQUAL(expected, stats.GetLogicalUsedBlocksCount());
    }

    Y_UNIT_TEST(ShouldCorrectlyCopyUsedBlocksCountForOverlayDisk)
    {
        ui32 blocksCount =  1024 * 1024 * 1024;
        ui32 usedBlocksCount = 0;

        TPartitionContent baseContent;
        for (size_t i = 0; i < blocksCount/4; i += 50) {
            baseContent.push_back(TBlob(i, 0, 49));
            usedBlocksCount += 49;
            baseContent.push_back(TEmpty());
        }

        auto config = DefaultConfig();
        config.SetLogicalUsedBlocksCalculationEnabled(true);

        auto partitionWithRuntime = SetupOverlayPartition(
            TestTabletId,
            TestTabletId2,
            baseContent,
            {},
            DefaultBlockSize,
            blocksCount,
            config);

        auto& partition = *partitionWithRuntime.Partition;

        auto response = partition.StatPartition();
        auto stats = response->Record.GetStats();

        UNIT_ASSERT_VALUES_EQUAL(0, stats.GetUsedBlocksCount());
        UNIT_ASSERT_VALUES_EQUAL(
            usedBlocksCount,
            stats.GetLogicalUsedBlocksCount()
        );
    }

    Y_UNIT_TEST(ShouldCorrectlyCalculateLogicalUsedBlocksCountForOverlayDisk)
    {
        ui32 blocksCount = 1024 * 128;
        TPartitionContent baseContent;
        for (size_t i = 0; i < 100; ++i) {
            baseContent.push_back(TEmpty());
        }
        for (size_t i = 100; i < 2048 + 100; ++i) {
            baseContent.push_back(TBlob(i, 1));
        }
        for (size_t i = 0; i < 100; ++i) {
            baseContent.push_back(TEmpty());
        }

        auto config = DefaultConfig();
        config.SetLogicalUsedBlocksCalculationEnabled(true);

        auto partitionWithRuntime = SetupOverlayPartition(
            TestTabletId,
            TestTabletId2,
            baseContent,
            {},
            DefaultBlockSize,
            blocksCount,
            config);

        auto& partition = *partitionWithRuntime.Partition;

        partition.WriteBlocks(TBlockRange32(1024 * 4, 1024 * 6), 1);  // +2049
        partition.ZeroBlocks(TBlockRange32(1024 * 5, 1024 * 5 + 10));  // -11
        partition.ZeroBlocks(TBlockRange32(1024 * 10, 1024 * 11));  // -0
        partition.WriteBlocks(TBlockRange32(1024 * 4, 1024 * 4 + 10), 1);  // +0
        partition.WriteBlocks(TBlockRange32(1024 * 3, 1024 * 3 + 10), 1); // +11

        ui64 expectedUsedBlocksCount = 2048 + 1 - 11 + 11;
        ui64 expectedLogicalUsedBlocksCount = 2048 + expectedUsedBlocksCount;

        auto response = partition.StatPartition();
        auto stats = response->Record.GetStats();

        UNIT_ASSERT_VALUES_EQUAL(
            expectedUsedBlocksCount,
            stats.GetUsedBlocksCount()
        );
        UNIT_ASSERT_VALUES_EQUAL(
            expectedLogicalUsedBlocksCount,
            stats.GetLogicalUsedBlocksCount()
        );

        partition.RebootTablet();

        response = partition.StatPartition();
        stats = response->Record.GetStats();

        UNIT_ASSERT_VALUES_EQUAL(
            expectedUsedBlocksCount,
            stats.GetUsedBlocksCount()
        );
        UNIT_ASSERT_VALUES_EQUAL(
            expectedLogicalUsedBlocksCount,
            stats.GetLogicalUsedBlocksCount()
        );

        // range [100-200) is zero on overlay disk, but non zero on base disk
        partition.ZeroBlocks(TBlockRange32(100, 199));

        response = partition.StatPartition();
        stats = response->Record.GetStats();

        UNIT_ASSERT_VALUES_EQUAL(
            expectedUsedBlocksCount,
            stats.GetUsedBlocksCount()
        );
        UNIT_ASSERT_VALUES_EQUAL(
            expectedLogicalUsedBlocksCount - 100,
            stats.GetLogicalUsedBlocksCount()
        );
    }

    Y_UNIT_TEST(ShouldReadBlocksFromBaseDisk)
    {
        TPartitionContent baseContent = {
        /*|      0      |     1     |     2 ... 5    |     6     |      7      |     8     |      9      |*/
            TBlob(1, 1) , TFresh(2) , TBlob(2, 3, 4) ,  TEmpty() , TBlob(1, 4) , TFresh(5) , TBlob(2, 6)
        };
        auto bitmap = CreateBitmap(10);

        auto partitionWithRuntime =
            SetupOverlayPartition(TestTabletId, TestTabletId2, baseContent);
        auto& partition = *partitionWithRuntime.Partition;
        auto& runtime = *partitionWithRuntime.Runtime;

        auto response = partition.ReadBlocks(TBlockRange32(0, 9));

        UNIT_ASSERT_VALUES_EQUAL(
            GetBlocksContent(baseContent), GetBlocksContent(response));
        UNIT_ASSERT(bitmap == GetUnencryptedBlockMask(response));

        const auto evStats = TEvStatsService::EvVolumePartCounters;

        ui32 externalBlobReads = 0;
        ui32 externalBlobBytes = 0;
        auto obs =
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                if (event->GetTypeRewrite() == evStats) {
                    auto* msg =
                        event->Get<TEvStatsService::TEvVolumePartCounters>();

                    const auto& readBlobCounters =
                        msg->DiskCounters->RequestCounters.ReadBlob;
                    externalBlobReads = readBlobCounters.ExternalCount;
                    externalBlobBytes = readBlobCounters.ExternalRequestBytes;
                }

                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            };

        runtime.SetObserverFunc(obs);

        partition.SendToPipe(
            std::make_unique<TEvPartitionPrivate::TEvUpdateCounters>());
        {
            TDispatchOptions options;
            options.FinalEvents.emplace_back(evStats);
            runtime.DispatchEvents(options);
        }

        UNIT_ASSERT_VALUES_EQUAL(2, externalBlobReads);
        UNIT_ASSERT_VALUES_EQUAL(7 * DefaultBlockSize, externalBlobBytes);
    }

    Y_UNIT_TEST(ShouldReadBlocksFromOverlayDisk)
    {
        TPartitionContent baseContent = {
            TBlob(1, 1), TFresh(2), TBlob(2, 3)
        };
        auto bitmap = CreateBitmap(3);

        auto partitionWithRuntime =
            SetupOverlayPartition(TestTabletId, TestTabletId2, baseContent);
        auto& partition = *partitionWithRuntime.Partition;

        const auto range = TBlockRange32::WithLength(0, baseContent.size());

        partition.WriteBlocks(range, char(2));
        MarkWrittenBlocks(bitmap, range);

        partition.Flush();

        auto response = partition.ReadBlocks(range);

        UNIT_ASSERT_VALUES_EQUAL(
            GetBlocksContent(char(2), range.Size()),
            GetBlocksContent(response));
        UNIT_ASSERT(bitmap == GetUnencryptedBlockMask(response));
    }

    Y_UNIT_TEST(ShouldReadBlocksFromOverlayDiskWhenRangeOverlapsWithBaseDisk)
    {
        TPartitionContent baseContent = {
        /*|      0      |     1     |      2      |     3     |      4      |     5     |      6      |*/
            TBlob(1, 1) , TFresh(1) , TBlob(2, 2) ,  TEmpty() , TBlob(3, 3) , TFresh(3) , TBlob(4, 3)
        };
        auto bitmap = CreateBitmap(7);

        auto partitionWithRuntime =
            SetupOverlayPartition(TestTabletId, TestTabletId2, baseContent);
        auto& partition = *partitionWithRuntime.Partition;

        TBlockRange32 writeRange(2, 4);
        partition.WriteBlocks(writeRange, char(4));
        MarkWrittenBlocks(bitmap, writeRange);

        partition.Flush();

        auto response = partition.ReadBlocks(TBlockRange32(0, 6));

        UNIT_ASSERT_VALUES_EQUAL(
            GetBlocksContent(char(1), 2) +
            GetBlocksContent(char(4), 3) +
            GetBlocksContent(char(3), 2),
            GetBlocksContent(response));
        UNIT_ASSERT(bitmap == GetUnencryptedBlockMask(response));
    }

    Y_UNIT_TEST(ShouldNotReadBlocksFromBaseDiskWhenTheyAreZeroedInOverlayDisk)
    {
        TPartitionContent baseContent = {
            TBlob(1, 1), TBlob(1, 2), TBlob(1, 3)
        };
        auto bitmap = CreateBitmap(3);

        auto partitionWithRuntime =
            SetupOverlayPartition(TestTabletId, TestTabletId2, baseContent);
        auto& partition = *partitionWithRuntime.Partition;

        const auto range = TBlockRange32::WithLength(0, baseContent.size());

        partition.ZeroBlocks(range);

        partition.Flush();

        auto response = partition.ReadBlocks(range);

        UNIT_ASSERT_VALUES_EQUAL(TString(), GetBlocksContent(response));
        UNIT_ASSERT(bitmap == GetUnencryptedBlockMask(response));
    }

    Y_UNIT_TEST(ShouldReadBlocksWithBaseDiskAndComplexRanges)
    {
        TPartitionContent baseContent = {
        /*|     0    |      1      |     2     |     3     |     4    |     5    |      6      |     7     |     8    |*/
            TEmpty() , TBlob(1, 1) , TFresh(2) ,  TEmpty() , TEmpty() , TEmpty() , TBlob(2, 3) , TFresh(4) , TEmpty()
        };
        auto bitmap = CreateBitmap(9);

        auto partitionWithRuntime =
            SetupOverlayPartition(TestTabletId, TestTabletId2, baseContent);
        auto& partition = *partitionWithRuntime.Partition;

        TBlockRange32 writeRange1(2, 3);
        partition.WriteBlocks(writeRange1, char(5));
        MarkWrittenBlocks(bitmap, writeRange1);

        TBlockRange32 writeRange2(5, 6);
        partition.WriteBlocks(writeRange2, char(6));
        MarkWrittenBlocks(bitmap, writeRange2);

        partition.Flush();

        auto response = partition.ReadBlocks(TBlockRange32(0, 8));

        UNIT_ASSERT_VALUES_EQUAL(
            GetBlocksContent(char(0), 1) +
            GetBlocksContent(char(1), 1) +
            GetBlocksContent(char(5), 2) +
            GetBlocksContent(char(0), 1) +
            GetBlocksContent(char(6), 2) +
            GetBlocksContent(char(4), 1) +
            GetBlocksContent(char(0), 1),
            GetBlocksContent(response));
        UNIT_ASSERT(bitmap == GetUnencryptedBlockMask(response));
    }

    Y_UNIT_TEST(ShouldReadBlocksAfterCompactionOfOverlayDisk1)
    {
        TPartitionContent baseContent = { TBlob(1, 1) };
        auto bitmap = CreateBitmap(1);

        auto partitionWithRuntime =
            SetupOverlayPartition(TestTabletId, TestTabletId2, baseContent);
        auto& partition = *partitionWithRuntime.Partition;

        // Write 1023 blocks (4MB minus 4KB). After compaction, we have one
        // merged blob written.
        // It's a tricky situation because one block (at 0 index) is missing in
        // overlay disk and therefore this block should be read from base disk.
        TBlockRange32 writeRange(1, 1023);
        partition.WriteBlocks(writeRange, char(2));
        MarkWrittenBlocks(bitmap, writeRange);

        partition.Compaction();

        auto response = partition.ReadBlocks(TBlockRange32(0, 1023));

        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(char(1)) +
            GetBlocksContent(char(2), 1023),
            GetBlocksContent(response));
        UNIT_ASSERT(bitmap == GetUnencryptedBlockMask(response));
    }

    Y_UNIT_TEST(ShouldReadBlocksAfterCompactionOfOverlayDisk2)
    {
        TPartitionContent baseContent;
        for (size_t i = 0; i < 1000; ++i) {
            baseContent.push_back(TEmpty());
        }
        for (size_t i = 1000; i < 1024; ++i) {
            baseContent.push_back(TBlob(i, 1));
        }
        auto bitmap = CreateBitmap(1024);

        auto partitionWithRuntime =
            SetupOverlayPartition(TestTabletId, TestTabletId2, baseContent);
        auto& partition = *partitionWithRuntime.Partition;

        // Write 1000 blocks. After compaction, we have one merged blob written.
        // It's a tricky situation because some blocks (in [1000..1023] range)
        // are missing in overlay disk and therefore these blocks should be read
        // from base disk.
        TBlockRange32 writeRange(0, 999);
        partition.WriteBlocks(writeRange, char(2));
        MarkWrittenBlocks(bitmap, writeRange);

        partition.Compaction();

        auto response = partition.ReadBlocks(TBlockRange32(0, 1023));

        UNIT_ASSERT_VALUES_EQUAL(
            GetBlocksContent(char(2), 1000) +
            GetBlocksContent(char(1), 24),
            GetBlocksContent(response));
        UNIT_ASSERT(bitmap == GetUnencryptedBlockMask(response));

        partition.WriteBlocks(TBlockRange32(0, 1023), char(3));
        partition.Compaction();
        partition.Cleanup();
        partition.CollectGarbage();

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            // Other blobs should be collected.
            UNIT_ASSERT_VALUES_EQUAL(1, stats.GetMergedBlobsCount());
        }
    }

    Y_UNIT_TEST(ShouldReadBlocksAfterZeroBlocksAndCompactionOfOverlayDisk)
    {
        TPartitionContent baseContent = { TBlob(1, 1) };
        auto bitmap = CreateBitmap(1);

        auto partitionWithRuntime =
            SetupOverlayPartition(TestTabletId, TestTabletId2, baseContent);
        auto& partition = *partitionWithRuntime.Partition;

        // Zero 1023 blocks. After compaction, we have one merged zero blob
        // written. It's a tricky situation because one block (at 0 index) is
        // missed in overlay disk and therefore this block should be read from
        // base disk.
        TBlockRange32 zeroRange(1, 1023);
        partition.ZeroBlocks(zeroRange);
        MarkZeroedBlocks(bitmap, zeroRange);

        partition.Compaction();

        auto response = partition.ReadBlocks(TBlockRange32(0, 1023));

        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(char(1)) +
            GetBlocksContent(char(0), 1023),
            GetBlocksContent(response));
        UNIT_ASSERT(bitmap == GetUnencryptedBlockMask(response));
    }

    Y_UNIT_TEST(ShouldNotKillTabletWhenFailedToReadBlobFromBaseDisk)
    {
        TPartitionContent baseContent = { TBlob(1, 1) };

        const auto baseTabletId = TestTabletId2;

        auto partitionWithRuntime =
            SetupOverlayPartition(TestTabletId, baseTabletId, baseContent);
        auto& partition = *partitionWithRuntime.Partition;
        auto& runtime = *partitionWithRuntime.Runtime;

        int pillCount = 0;

        const auto eventHandler =
            [&] (const TEvBlobStorage::TEvGet::TPtr& ev) {
                bool result = false;

                auto& msg = *ev->Get();

                auto response = std::make_unique<TEvBlobStorage::TEvGetResult>(
                    NKikimrProto::ERROR,
                    msg.QuerySize,
                    0);  // groupId

                for (ui32 i = 0; i < msg.QuerySize; ++i) {
                    const auto& q = msg.Queries[i];
                    const auto& blobId = q.Id;

                    if (blobId.TabletID() == baseTabletId) {
                        result = true;
                    }
                }

                if (result) {
                    runtime.Schedule(
                        new IEventHandle(
                            ev->Sender,
                            ev->Recipient,
                            response.release(),
                            0,
                            ev->Cookie),
                        TDuration());
                }

                return result;
            };

        runtime.SetEventFilter(
            [eventHandler] (TTestActorRuntimeBase&, TAutoPtr<IEventHandle>& ev) {
                bool handled = false;

                const auto wrapped =
                    [&] (const auto& ev) {
                        handled = eventHandler(ev);
                    };

                switch (ev->GetTypeRewrite()) {
                    hFunc(TEvBlobStorage::TEvGet, wrapped);
                }
                return handled;
           });

        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvents::TSystem::PoisonPill: {
                        ++pillCount;
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        partition.SendReadBlocksRequest(0);

        auto response = partition.RecvReadBlocksResponse();
        UNIT_ASSERT(FAILED(response->GetStatus()));

        UNIT_ASSERT_VALUES_EQUAL(0, pillCount);
    }

    Y_UNIT_TEST(ShouldReadBlocksWhenBaseBlobHasGreaterChannel)
    {
        const ui32 overlayTabletChannelsCount = 254;
        const ui32 baseBlobChannel = overlayTabletChannelsCount + 1;

        TPartitionContent baseContent = {
            TBlob(1, 1, 1, baseBlobChannel)
        };
        auto bitmap = CreateBitmap(1);

        auto partitionWithRuntime =
            SetupOverlayPartition(
                TestTabletId,
                TestTabletId2,
                baseContent,
                overlayTabletChannelsCount);
        auto& partition = *partitionWithRuntime.Partition;

        auto response = partition.ReadBlocks(0);

        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(char(1)),
            GetBlocksContent(response));
        UNIT_ASSERT(bitmap == GetUnencryptedBlockMask(response));
    }

    Y_UNIT_TEST(ShouldSendBlocksCountToReadInDescribeBlocksRequest)
    {
        auto partitionWithRuntime =
            SetupOverlayPartition(TestTabletId, TestTabletId2, {});
        auto& partition = *partitionWithRuntime.Partition;
        auto& runtime = *partitionWithRuntime.Runtime;

        partition.WriteBlocks(4, 1);
        partition.WriteBlocks(5, 1);

        int describeBlocksCount = 0;

        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvVolume::EvDescribeBlocksRequest: {
                        auto* msg = event->Get<TEvVolume::TEvDescribeBlocksRequest>();
                        auto& record = msg->Record;
                        UNIT_ASSERT_VALUES_EQUAL(
                            7,
                            record.GetBlocksCountToRead()
                        );
                        ++describeBlocksCount;
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        partition.ReadBlocks(TBlockRange32(0, 8));
        UNIT_ASSERT_VALUES_EQUAL(1, describeBlocksCount);
    }

    Y_UNIT_TEST(ShouldGetUsedBlocks)
    {
        constexpr ui32 blockCount = 1024 * 1024;
        auto runtime = PrepareTestActorRuntime(DefaultConfig(), blockCount);
        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(TBlockRange32(0, 1), 1);
        partition.WriteBlocks(TBlockRange32(0, 1023), 1);
        partition.WriteBlocks(TBlockRange32(2048, 2049), 1);
        partition.WriteBlocks(TBlockRange32(1024 * 10, 1024 * 11), 1);
        partition.WriteBlocks(TBlockRange32(1024 * 512, 1024 * 513), 1);

        ui64 expected = 1024 + 2 + 1024 + 1 + 1024 + 1;

        auto response1 = partition.GetUsedBlocks();
        UNIT_ASSERT(SUCCEEDED(response1->GetStatus()));

        TCompressedBitmap bitmap(blockCount);

        for (const auto& block : response1->Record.GetUsedBlocks()) {
            bitmap.Merge(TCompressedBitmap::TSerializedChunk{
                block.GetChunkIdx(),
                block.GetData()
            });
        }

        UNIT_ASSERT_EQUAL(expected, bitmap.Count());
    }

    Y_UNIT_TEST(ShouldFailReadBlocksWhenSglistHolderIsDestroyed) {
        auto runtime = PrepareTestActorRuntime();
        TPartitionClient partition(*runtime);
        partition.WaitReady();
        partition.WriteBlocks(0, 1);

        // Test with fresh blocks.
        {
            TGuardedSgList sglist(TSgList{{}});
            sglist.Destroy();
            partition.SendReadBlocksLocalRequest(0, sglist);

            auto response = partition.RecvReadBlocksLocalResponse();
            UNIT_ASSERT(FAILED(response->GetStatus()));
        }

        partition.Flush();

        // Test with blob.
        {
            TGuardedSgList sglist(TSgList{{}});
            sglist.Destroy();
            partition.SendReadBlocksLocalRequest(0, sglist);

            auto response = partition.RecvReadBlocksLocalResponse();
            UNIT_ASSERT(FAILED(response->GetStatus()));
        }
    }

    Y_UNIT_TEST(ShouldReturnErrorWhenReadingFromUnknownCheckpoint)
    {
        auto runtime = PrepareTestActorRuntime();

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.SendReadBlocksRequest(TBlockRange32(0, 0), "unknown");

        auto response = partition.RecvReadBlocksResponse();
        UNIT_ASSERT(FAILED(response->GetStatus()));
    }

    Y_UNIT_TEST(ShouldReturnErrorIfCompactionIsAlreadyRunning)
    {
        auto runtime = PrepareTestActorRuntime(DefaultConfig(), 2048);

        runtime->SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvPartitionPrivate::EvCompactionCompleted:
                    case TEvPartitionPrivate::EvCompactionResponse: {
                        return TTestActorRuntime::EEventAction::DROP ;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });


        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(TBlockRange32(0, 1023));
        partition.WriteBlocks(TBlockRange32(1024, 2047));

        partition.SendCompactionRequest(0);

        partition.SendCompactionRequest(1024);

        auto compactResponse1 = partition.RecvCompactionResponse();
        UNIT_ASSERT(FAILED(compactResponse1->GetStatus()));
        UNIT_ASSERT_VALUES_EQUAL(E_TRY_AGAIN, compactResponse1->GetStatus());
    }

    Y_UNIT_TEST(ShouldStartCompactionOnCompactRagesRequest)
    {
        auto runtime = PrepareTestActorRuntime();

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(0, 100);
        partition.SendCompactRangeRequest(0, 100);

        auto response = partition.RecvCompactRangeResponse();
        UNIT_ASSERT(SUCCEEDED(response->GetStatus()));
        UNIT_ASSERT_VALUES_EQUAL(
            false,
            response->Record.GetOperationId().empty()
        );
    }

    Y_UNIT_TEST(ShouldReturnErrorForUnknownIdInCompactionStatusRequest)
    {
        auto runtime = PrepareTestActorRuntime();

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(0, 100);
        partition.SendGetCompactionStatusRequest("xxx");

        auto response = partition.RecvGetCompactionStatusResponse();
        UNIT_ASSERT(FAILED(response->GetStatus()));
    }

    Y_UNIT_TEST(ShouldReturnCompactionProgress)
    {
        auto runtime = PrepareTestActorRuntime();

        TActorId rangeActor;
        TActorId partActor;
        runtime->SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvPartitionPrivate::EvCompactionRequest: {
                        rangeActor = event->Sender;
                        partActor = event->Recipient;
                        return TTestActorRuntime::EEventAction::DROP ;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });


        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(0, 100);
        auto compResponse = partition.CompactRange(0, 100);

        partition.SendGetCompactionStatusRequest(compResponse->Record.GetOperationId());

        auto statusResponse1 = partition.RecvGetCompactionStatusResponse();
        UNIT_ASSERT(SUCCEEDED(statusResponse1->GetStatus()));
        UNIT_ASSERT_VALUES_EQUAL(
            false,
            statusResponse1->Record.GetIsCompleted()
        );
        UNIT_ASSERT_VALUES_EQUAL(1, statusResponse1->Record.GetTotal());

        auto compactionResponse = std::make_unique<TEvPartitionPrivate::TEvCompactionResponse>();
        runtime->Send(
            new IEventHandle(
                rangeActor,
                partActor,
                compactionResponse.release(),
                0, // flags
                0),
            0);

        partition.SendGetCompactionStatusRequest(compResponse->Record.GetOperationId());

        auto statusResponse2 = partition.RecvGetCompactionStatusResponse();
        UNIT_ASSERT(SUCCEEDED(statusResponse2->GetStatus()));
        UNIT_ASSERT_VALUES_EQUAL(true, statusResponse2->Record.GetIsCompleted());
        UNIT_ASSERT_VALUES_EQUAL(1, statusResponse2->Record.GetTotal());
    }

    Y_UNIT_TEST(ShouldReturnCompactionStatusForLastCompletedCompaction)
    {
        auto runtime = PrepareTestActorRuntime();

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(0, 100);
        auto compResponse = partition.CompactRange(0, 100);

        auto statusResponse = partition.GetCompactionStatus(
            compResponse->Record.GetOperationId());

        UNIT_ASSERT_VALUES_EQUAL(true, statusResponse->Record.GetIsCompleted());
        UNIT_ASSERT_VALUES_EQUAL(1, statusResponse->Record.GetTotal());
    }

    Y_UNIT_TEST(ShouldUseWriteBlobThreshold)
    {
        auto config = DefaultConfig();
        config.SetWriteBlobThreshold(1_MB);
        config.SetWriteBlobThresholdSSD(128_KB);
        TTestPartitionInfo testPartitionInfo;

        const auto mediaKinds = {
            NCloud::NProto::STORAGE_MEDIA_HDD,
            NCloud::NProto::STORAGE_MEDIA_HYBRID
        };

        for (auto mediaKind: mediaKinds) {
            testPartitionInfo.MediaKind = mediaKind;
            auto runtime = PrepareTestActorRuntime(
                config,
                1024,
                {},
                testPartitionInfo,
                {},
                EStorageAccessMode::Default
            );

            TPartitionClient partition(*runtime);
            partition.WaitReady();

            partition.WriteBlocks(TBlockRange32(0, 254));

            {
                auto response = partition.StatPartition();
                const auto& stats = response->Record.GetStats();
                UNIT_ASSERT_VALUES_EQUAL(255, stats.GetFreshBlocksCount());
                UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlocksCount());
                UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMergedBlocksCount());
            }

            partition.WriteBlocks(TBlockRange32(0, 255));

            {
                auto response = partition.StatPartition();
                const auto& stats = response->Record.GetStats();
                UNIT_ASSERT_VALUES_EQUAL(255, stats.GetFreshBlocksCount());
                UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlocksCount());
                UNIT_ASSERT_VALUES_EQUAL(256, stats.GetMergedBlocksCount());
            }

            partition.ZeroBlocks(TBlockRange32(0, 254));

            {
                auto response = partition.StatPartition();
                const auto& stats = response->Record.GetStats();
                UNIT_ASSERT_VALUES_EQUAL(255, stats.GetFreshBlocksCount());
                UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlocksCount());
                UNIT_ASSERT_VALUES_EQUAL(256, stats.GetMergedBlocksCount());
            }

            partition.ZeroBlocks(TBlockRange32(0, 255));

            {
                auto response = partition.StatPartition();
                const auto& stats = response->Record.GetStats();
                UNIT_ASSERT_VALUES_EQUAL(255, stats.GetFreshBlocksCount());
                UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlocksCount());
                UNIT_ASSERT_VALUES_EQUAL(256, stats.GetMergedBlocksCount());
            }
        }

        {
            testPartitionInfo.MediaKind = NCloud::NProto::STORAGE_MEDIA_SSD;
            auto runtime = PrepareTestActorRuntime(
                config,
                1024,
                {},
                testPartitionInfo,
                {},
                EStorageAccessMode::Default
            );

            TPartitionClient partition(*runtime);
            partition.WaitReady();

            partition.WriteBlocks(TBlockRange32(0, 30));

            {
                auto response = partition.StatPartition();
                const auto& stats = response->Record.GetStats();
                UNIT_ASSERT_VALUES_EQUAL(31, stats.GetFreshBlocksCount());
                UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlocksCount());
                UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMergedBlocksCount());
            }

            partition.WriteBlocks(TBlockRange32(0, 31));

            {
                auto response = partition.StatPartition();
                const auto& stats = response->Record.GetStats();
                UNIT_ASSERT_VALUES_EQUAL(31, stats.GetFreshBlocksCount());
                UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlocksCount());
                UNIT_ASSERT_VALUES_EQUAL(32, stats.GetMergedBlocksCount());
            }

            partition.ZeroBlocks(TBlockRange32(0, 30));

            {
                auto response = partition.StatPartition();
                const auto& stats = response->Record.GetStats();
                UNIT_ASSERT_VALUES_EQUAL(31, stats.GetFreshBlocksCount());
                UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlocksCount());
                UNIT_ASSERT_VALUES_EQUAL(32, stats.GetMergedBlocksCount());
            }

            partition.ZeroBlocks(TBlockRange32(0, 31));

            {
                auto response = partition.StatPartition();
                const auto& stats = response->Record.GetStats();
                UNIT_ASSERT_VALUES_EQUAL(31, stats.GetFreshBlocksCount());
                UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlocksCount());
                UNIT_ASSERT_VALUES_EQUAL(32, stats.GetMergedBlocksCount());
            }
        }
    }

    Y_UNIT_TEST(ShouldProperlyProcessDeletionMarkers)
    {
        auto config = DefaultConfig();
        config.SetWriteBlobThreshold(1_MB);

        auto runtime = PrepareTestActorRuntime(config);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(TBlockRange32(0, 1023));
        partition.ZeroBlocks(TBlockRange32(0, 1023));

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlobsCount());

            UNIT_ASSERT_VALUES_EQUAL(1024, stats.GetMergedBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(2, stats.GetMergedBlobsCount());
        }

        partition.Compaction();
        partition.Cleanup();

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlobsCount());

            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMergedBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(1, stats.GetMergedBlobsCount());
        }

        partition.WriteBlocks(TBlockRange32(0, 99));
        partition.ZeroBlocks(TBlockRange32(0, 99));
        partition.Flush();
        partition.Flush();

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();

            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(1, stats.GetMixedBlobsCount());

            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMergedBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(1, stats.GetMergedBlobsCount());
        }

        partition.Compaction();
        partition.Cleanup();

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMixedBlobsCount());

            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetMergedBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(1, stats.GetMergedBlobsCount());
        }
    }

    Y_UNIT_TEST(ShouldHandleHttpCollectGarbage)
    {
        auto runtime = PrepareTestActorRuntime();

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        auto createResponse = partition.RemoteHttpInfo(
            BuildRemoteHttpQuery(TestTabletId, {{"action","collectGarbage"}}),
            HTTP_METHOD::HTTP_METHOD_POST);

        UNIT_ASSERT_C(
            createResponse->Html.Contains("Operation successfully completed"),
            true
        );
    }

    Y_UNIT_TEST(ShouldFailsHttpGetCollectGarbage)
    {
        auto runtime = PrepareTestActorRuntime();

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        auto createResponse = partition.RemoteHttpInfo(
            BuildRemoteHttpQuery(TestTabletId, {{"action","collectGarbage"}}));

        UNIT_ASSERT_C(createResponse->Html.Contains("Wrong HTTP method"), true);
    }

    Y_UNIT_TEST(ShouldFailHttpCollectGarbageOnTabletRestart)
    {
        auto runtime = PrepareTestActorRuntime();

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        bool patchRequest = true;
        runtime->SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                if (event->GetTypeRewrite() == TEvPartitionPrivate::EvCollectGarbageRequest) {
                    if (patchRequest) {
                        patchRequest = false;
                        auto request = std::make_unique<TEvPartitionPrivate::TEvCollectGarbageRequest>();
                        SendUndeliverableRequest(runtime, event, std::move(request));
                        return TTestActorRuntime::EEventAction::DROP;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        auto httpResponse = partition.RemoteHttpInfo(
            BuildRemoteHttpQuery(TestTabletId, {{"action","collectGarbage"}}),
            HTTP_METHOD::HTTP_METHOD_POST);

        UNIT_ASSERT_C(httpResponse->Html.Contains("Tablet is dead"), true);
    }

    Y_UNIT_TEST(ShouldForgetTooOldCompactRangeOperations)
    {
        constexpr TDuration CompactOpHistoryDuration = TDuration::Days(1);

        auto runtime = PrepareTestActorRuntime();

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        TVector<TString> op;

        for (ui32 i = 0; i < 4; ++i)
        {
            partition.WriteBlocks(0, 100);
            auto compResponse = partition.CompactRange(0, 100);
            op.push_back(compResponse->Record.GetOperationId());
        }

        for (ui32 i = 0; i < 4; ++i)
        {
            partition.GetCompactionStatus(op[i]);
        }

        runtime->AdvanceCurrentTime(CompactOpHistoryDuration + TDuration::Seconds(1));

        for (ui32 i = 0; i < 4; ++i)
        {
            partition.SendGetCompactionStatusRequest(op[i]);
            auto response = partition.RecvGetCompactionStatusResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_NOT_FOUND, response->GetStatus());
        }
    }

    Y_UNIT_TEST(ShouldDrain)
    {
        auto config = DefaultConfig();
        config.SetWriteBlobThreshold(1_MB);
        auto runtime = PrepareTestActorRuntime(config);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        // drain before any requests should work
        partition.Drain();

        partition.WriteBlocks(TBlockRange32(0));          // fresh
        partition.WriteBlocks(TBlockRange32(0, 1023));    // blob
        partition.ZeroBlocks(TBlockRange32(0));           // fresh
        partition.ZeroBlocks(TBlockRange32(0, 1023));     // blob

        // drain after some requests have completed should work
        partition.Drain();

        TDeque<std::unique_ptr<IEventHandle>> evPutRequests;
        bool intercept = true;
        runtime->SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event)
            {
                if (intercept) {
                    switch (event->GetTypeRewrite()) {
                        case TEvBlobStorage::EvPutResult: {
                            evPutRequests.emplace_back(event.Release());
                            return TTestActorRuntime::EEventAction::DROP;
                        }
                    }
                }

                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            }
        );

        auto test = [&] (TString testName, bool isWrite)
        {
            runtime->DispatchEvents({}, TDuration::Seconds(1));

            // sending this request after DispatchEvents because our pipe client
            // reconnects from time to time and thus fifo guarantee for
            // zero/write -> drain is broken => we need to ensure the correct
            // order (zero/write, then drain) in our test scenario
            partition.SendDrainRequest();

            UNIT_ASSERT_C(
                evPutRequests.size(),
                TStringBuilder() << testName << ": intercepted EvPut requests"
            );

            auto evList = runtime->CaptureEvents();
            for (auto& ev: evList) {
                UNIT_ASSERT_C(
                    ev->GetTypeRewrite() != TEvPartition::EvDrainResponse,
                    TStringBuilder() << testName << ": check no drain response"
                );
            }
            runtime->PushEventsFront(evList);

            intercept = false;

            for (auto& request: evPutRequests) {
                runtime->Send(request.release());
            }

            evPutRequests.clear();

            runtime->DispatchEvents({}, TDuration::Seconds(1));

            if (isWrite) {
                {
                    auto response = partition.RecvWriteBlocksResponse();
                    UNIT_ASSERT_VALUES_EQUAL(S_OK, response->GetStatus());
                }
            } else {
                {
                    auto response = partition.RecvZeroBlocksResponse();
                    UNIT_ASSERT_VALUES_EQUAL(S_OK, response->GetStatus());
                }
            }

            {
                auto response = partition.RecvDrainResponse();
                UNIT_ASSERT_VALUES_EQUAL_C(
                    S_OK,
                    response->GetStatus(),
                    TStringBuilder() << testName << ": check drain response"
                );
            }

            intercept = true;
        };

        partition.SendWriteBlocksRequest(TBlockRange32(0));
        test("write fresh", true);

        partition.SendWriteBlocksRequest(TBlockRange32(0, 1023));
        test("write blob", true);

        partition.SendZeroBlocksRequest(TBlockRange32(0));
        test("zero fresh", false);

        partition.SendZeroBlocksRequest(TBlockRange32(0, 1023));
        test("zero blob", false);
    }

    Y_UNIT_TEST(ShouldProperlyHandleCollectGarbageErrors)
    {
        const auto channelCount = 6;
        const auto groupCount = channelCount - DataChannelOffset;

        TTestEnv env(0, 1, channelCount, groupCount);
        auto& runtime = env.GetRuntime();
        auto tabletId = InitTestActorRuntime(env, runtime, channelCount, channelCount);

        TPartitionClient partition(runtime, 0, tabletId);
        partition.WaitReady();

        bool channel3requestObserved = false;
        bool channel4requestObserved = false;
        bool channel4responseObserved = false;
        bool deleteGarbageObserved = false;
        bool sendError = true;

        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvBlobStorage::EvCollectGarbage: {
                        auto* msg = event->Get<TEvBlobStorage::TEvCollectGarbage>();
                        if (3 == msg->Channel) {
                            channel3requestObserved = true;
                            if (sendError) {
                                auto response =
                                    std::make_unique<TEvBlobStorage::TEvCollectGarbageResult>(
                                        NKikimrProto::ERROR,
                                        0,  // doesn't matter
                                        0,  // doesn't matter
                                        0,  // doesn't matter
                                        msg->Channel
                                    );

                                runtime.Send(new IEventHandle(
                                    event->Sender,
                                    event->Recipient,
                                    response.release(),
                                    0, // flags
                                    0
                                ), 0);

                                return TTestActorRuntime::EEventAction::DROP;
                            }
                        } else if (4 == msg->Channel) {
                            channel4requestObserved = true;
                        }

                        break;
                    }

                    case TEvBlobStorage::EvCollectGarbageResult: {
                        auto* msg = event->Get<TEvBlobStorage::TEvCollectGarbageResult>();
                        if (4 == msg->Channel) {
                            channel4responseObserved = true;
                        }

                        break;
                    }

                    case TEvPartitionPrivate::EvDeleteGarbageRequest: {
                        deleteGarbageObserved = true;

                        break;
                    }
                }

                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            }
        );

        // 10 blobs needed to trigger automatic collect
        for (ui32 i = 0; i < 9; ++i) {
            partition.WriteBlocks(TBlockRange32(0, 1023), 1);
        }
        partition.Compaction();
        partition.Cleanup();
        partition.SendCollectGarbageRequest();
        {
            auto response = partition.RecvCollectGarbageResponse();
            UNIT_ASSERT_VALUES_EQUAL(
                MAKE_KIKIMR_ERROR(NKikimrProto::ERROR),
                response->GetStatus()
            );
        }
        UNIT_ASSERT(channel3requestObserved);
        UNIT_ASSERT(channel4requestObserved);
        UNIT_ASSERT(channel4responseObserved);
        UNIT_ASSERT(!deleteGarbageObserved);
        channel3requestObserved = false;
        channel4requestObserved = false;
        channel4responseObserved = false;
        sendError = false;

        runtime.AdvanceCurrentTime(TDuration::Seconds(10));
        runtime.DispatchEvents(TDispatchOptions(), TDuration::MilliSeconds(10));

        UNIT_ASSERT(channel3requestObserved);
        UNIT_ASSERT(channel4requestObserved);
        UNIT_ASSERT(channel4responseObserved);
        UNIT_ASSERT(deleteGarbageObserved);
    }

    Y_UNIT_TEST(ShouldExecuteCollectGarbageAtStartup)
    {
        const auto channelCount = 7;
        const auto groupCount = channelCount - DataChannelOffset;

        auto isDataChannel = [&] (ui64 ch) {
            return (ch >= DataChannelOffset) && (ch < channelCount);
        };

        NProto::TStorageServiceConfig config = DefaultConfig();

        TTestEnv env(0, 1, channelCount, groupCount);
        auto& runtime = env.GetRuntime();
        const auto tabletId = InitTestActorRuntime(env, runtime, channelCount, channelCount, config);

        TPartitionClient partition(runtime, 0, tabletId);

        ui32 gcRequests = 0;
        bool deleteGarbageSeen = false;

        NActors::TActorId gcActor = {};

        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvBlobStorage::EvCollectGarbage: {
                        auto* msg = event->Get<TEvBlobStorage::TEvCollectGarbage>();
                        if (msg->TabletId == tabletId &&
                            isDataChannel(msg->Channel))
                        {
                            if (!gcActor || gcActor == event->Sender) {
                                gcActor = event->Sender;
                                ++gcRequests;
                            }
                        }
                        break;
                    }
                    case TEvPartitionPrivate::EvDeleteGarbageRequest: {
                        deleteGarbageSeen = true;
                        break;
                    }
                }

                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            }
        );

        {
            TDispatchOptions options;
            options.FinalEvents.emplace_back(TEvPartitionPrivate::EvCollectGarbageResponse);
            runtime.DispatchEvents(options);
        }

        UNIT_ASSERT_VALUES_EQUAL(3, gcRequests);
        UNIT_ASSERT_VALUES_EQUAL(false, deleteGarbageSeen);
    }

    Y_UNIT_TEST(ShouldNotTrimInFlightBlocks)
    {
        auto config = DefaultConfig();
        config.SetFreshChannelCount(1);
        config.SetFreshChannelWriteRequestsEnabled(true);

        auto runtime = PrepareTestActorRuntime(config);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(1, 1);

        TAutoPtr<IEventHandle> addFreshBlocks;

        runtime->SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event)
            {
                switch (event->GetTypeRewrite()) {
                    case TEvPartitionPrivate::EvAddFreshBlocksRequest: {
                        if (!addFreshBlocks) {
                            addFreshBlocks = event.Release();
                            return TTestActorRuntime::EEventAction::DROP;
                        }
                        break;
                    }
                }

                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            }
        );

        partition.SendWriteBlocksRequest(2, 2);

        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        partition.WriteBlocks(3, 3);

        partition.Flush();
        partition.TrimFreshLog();

        UNIT_ASSERT(addFreshBlocks);
        runtime->Send(addFreshBlocks.Release());

        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        partition.RebootTablet();

        auto response = partition.ReadBlocks(1);
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(char(1)),
            GetBlocksContent(response)
        );

        response = partition.ReadBlocks(2);
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(char(2)),
            GetBlocksContent(response)
        );

        response = partition.ReadBlocks(3);
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(char(3)),
            GetBlocksContent(response)
        );
    }

    Y_UNIT_TEST(ShouldNotTrimUnflushedBlocksWhileThereIsFlushedBlockWithLargerCommitId)
    {
        auto config = DefaultConfig();
        config.SetFreshChannelCount(1);
        config.SetFreshChannelWriteRequestsEnabled(true);

        auto runtime = PrepareTestActorRuntime(config);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(1, 1);

        TAutoPtr<IEventHandle> addFreshBlocks;

        runtime->SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event)
            {
                switch (event->GetTypeRewrite()) {
                    case TEvPartitionPrivate::EvAddFreshBlocksRequest: {
                        if (!addFreshBlocks) {
                            addFreshBlocks = event.Release();
                            return TTestActorRuntime::EEventAction::DROP;
                        }
                        break;
                    }
                }

                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            }
        );

        partition.SendWriteBlocksRequest(2, 2);

        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        partition.WriteBlocks(3, 3);

        partition.Flush();

        UNIT_ASSERT(addFreshBlocks);
        runtime->Send(addFreshBlocks.Release());

        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        partition.TrimFreshLog();

        partition.RebootTablet();

        auto response = partition.ReadBlocks(1);
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(char(1)),
            GetBlocksContent(response)
        );

        response = partition.ReadBlocks(2);
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(char(2)),
            GetBlocksContent(response)
        );

        response = partition.ReadBlocks(3);
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(char(3)),
            GetBlocksContent(response)
        );
    }

    Y_UNIT_TEST(ShouldReleaseTrimBarrierOnBlockDeletion)
    {
        auto config = DefaultConfig();
        config.SetFreshChannelCount(1);
        config.SetFreshChannelWriteRequestsEnabled(true);

        auto runtime = PrepareTestActorRuntime(config);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(TBlockRange32(1, 2));
        partition.WriteBlocks(TBlockRange32(1, 2));

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(2, stats.GetFreshBlocksCount());
        }

        partition.Flush();
        partition.TrimFreshLog();

        partition.RebootTablet();

        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetFreshBlocksCount());
        }
    }

    Y_UNIT_TEST(ShouldHandleFlushCorrectlyWhileBlockFromFreshChannelIsBeingDeleted)
    {
        auto config = DefaultConfig();
        config.SetFreshChannelCount(1);
        config.SetFreshChannelWriteRequestsEnabled(true);

        auto runtime = PrepareTestActorRuntime(config);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(TBlockRange32(1, 4));

        TAutoPtr<IEventHandle> addBlobsRequest;

        runtime->SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event)
            {
                if (event->GetTypeRewrite() == TEvPartitionPrivate::EvAddBlobsRequest) {
                    addBlobsRequest = event.Release();
                    return TTestActorRuntime::EEventAction::DROP;
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            }
        );

        partition.SendFlushRequest();

        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        partition.WriteBlocks(TBlockRange32(1, 4));

        UNIT_ASSERT(addBlobsRequest);
        runtime->Send(addBlobsRequest.Release());

        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        auto response = partition.RecvFlushResponse();
        UNIT_ASSERT(SUCCEEDED(response->GetStatus()));
    }

    Y_UNIT_TEST(ShouldHandleFlushCorrectlyWhileBlockFromDbIsBeingDeleted)
    {
        auto config = DefaultConfig();
        config.SetFreshChannelCount(0);
        config.SetFreshChannelWriteRequestsEnabled(false);

        auto runtime = PrepareTestActorRuntime(config);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(TBlockRange32(1, 4));

        TAutoPtr<IEventHandle> addBlobsRequest;

        runtime->SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event)
            {
                if (event->GetTypeRewrite() == TEvPartitionPrivate::EvAddBlobsRequest) {
                    addBlobsRequest = event.Release();
                    return TTestActorRuntime::EEventAction::DROP;
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            }
        );

        partition.SendFlushRequest();

        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        partition.WriteBlocks(TBlockRange32(1, 4));

        UNIT_ASSERT(addBlobsRequest);
        runtime->Send(addBlobsRequest.Release());

        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        auto response = partition.RecvFlushResponse();
        UNIT_ASSERT(SUCCEEDED(response->GetStatus()));
    }

    Y_UNIT_TEST(ShouldCorrectlyHandleTabletInfoChannelCountMoreThanConfigChannelCount)
    {
        constexpr ui32 channelCount = 7;
        constexpr ui32 tabletInfoChannelCount = 11;

        TTestEnv env(0, 1, tabletInfoChannelCount, 4);
        auto& runtime = env.GetRuntime();

        const auto tabletId = InitTestActorRuntime(
            env,
            runtime,
            channelCount,
            tabletInfoChannelCount);

        TPartitionClient partition(runtime, 0, tabletId);
        partition.WaitReady();

        for (ui32 i = 0; i < 30; ++i) {
            partition.WriteBlocks(TBlockRange32(0, 1023), 1);
        }

        partition.Compaction();
        partition.CollectGarbage();
    }

    Y_UNIT_TEST(ShouldCorrectlyHandleTabletInfoChannelCountLessThanConfigChannelCount)
    {
        constexpr ui32 channelCount = 11;
        constexpr ui32 tabletInfoChannelCount = 7;

        TTestEnv env(0, 1, tabletInfoChannelCount, 4);
        auto& runtime = env.GetRuntime();

        const auto tabletId = InitTestActorRuntime(
            env,
            runtime,
            channelCount,
            tabletInfoChannelCount);

        TPartitionClient partition(runtime, 0, tabletId);
        partition.WaitReady();

        for (ui32 i = 0; i < 30; ++i) {
            partition.WriteBlocks(TBlockRange32(0, 1023), 1);
        }

        partition.Compaction();
        partition.CollectGarbage();
    }

    Y_UNIT_TEST(ShouldHandleBSErrorsOnInitFreshBlocksFromChannel)
    {
        auto config = DefaultConfig();
        config.SetFreshChannelCount(1);
        config.SetFreshChannelWriteRequestsEnabled(true);

        auto runtime = PrepareTestActorRuntime(config);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(TBlockRange32(0), 1);
        partition.WriteBlocks(TBlockRange32(1), 2);
        partition.WriteBlocks(TBlockRange32(2), 3);

        bool evRangeResultSeen = false;

        ui32 evLoadFreshBlobsCompletedCount = 0;

        runtime->SetEventFilter(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event)
            {
                switch (event->GetTypeRewrite()) {
                    case TEvBlobStorage::EvRangeResult: {
                        using TEvent = TEvBlobStorage::TEvRangeResult;
                        auto* msg = event->Get<TEvent>();

                        if (msg->From.Channel() != 4 || evRangeResultSeen) {
                            return false;
                        }

                        evRangeResultSeen = true;

                        auto response = std::make_unique<TEvent>(
                            NKikimrProto::ERROR,
                            TLogoBlobID(),  // doesn't matter
                            TLogoBlobID(),  // doesn't matter
                            0);             // doesn't matter

                        auto* handle = new IEventHandle(
                            event->Recipient,
                            event->Sender,
                            response.release(),
                            0,
                            event->Cookie);

                        runtime.Send(handle, 0);

                        return true;
                    }
                    case TEvPartitionCommonPrivate::EvLoadFreshBlobsCompleted: {
                        ++evLoadFreshBlobsCompletedCount;
                        return false;
                    }
                    default: {
                        return false;
                    }
                }
            }
        );

        partition.RebootTablet();
        partition.WaitReady();

        UNIT_ASSERT_VALUES_EQUAL(true, evRangeResultSeen);

        // tablet rebooted twice (after explicit RebootTablet() and on fail)
        UNIT_ASSERT_VALUES_EQUAL(2, evLoadFreshBlobsCompletedCount);

        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(1),
            GetBlockContent(partition.ReadBlocks(0)));
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(2),
            GetBlockContent(partition.ReadBlocks(1)));
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(3),
            GetBlockContent(partition.ReadBlocks(2)));
    }

    Y_UNIT_TEST(ShouldFillEnryptedBlockMaskWhenReadBlock)
    {
        TBlockRange32 range(0, 15);
        auto emptyBlock = TString::Uninitialized(DefaultBlockSize);

        auto bitmap = CreateBitmap(16);

        auto runtime = PrepareTestActorRuntime();

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        {
            partition.Flush();

            auto response = partition.ReadBlocks(range);
            UNIT_ASSERT(bitmap == GetUnencryptedBlockMask(response));

            TVector<TString> blocks;
            auto sglist = ResizeBlocks(blocks, range.Size(), emptyBlock);
            auto localResponse = partition.ReadBlocksLocal(range, sglist);
            UNIT_ASSERT(bitmap == GetUnencryptedBlockMask(localResponse));
        }

        {
            TBlockRange32 writeRange(1, 3);
            partition.WriteBlocks(writeRange, char(4));
            MarkWrittenBlocks(bitmap, writeRange);

            TBlockRange32 zeroRange(5, 7);
            partition.ZeroBlocks(zeroRange);
            MarkZeroedBlocks(bitmap, zeroRange);

            TBlockRange32 writeRangeLocal(8, 10);
            partition.WriteBlocksLocal(writeRangeLocal, GetBlockContent(4));
            MarkWrittenBlocks(bitmap, writeRangeLocal);

            TBlockRange32 zeroRangeLocal(12, 14);
            partition.ZeroBlocks(zeroRangeLocal);
            MarkZeroedBlocks(bitmap, zeroRangeLocal);

            partition.Flush();

            auto response = partition.ReadBlocks(range);
            UNIT_ASSERT(bitmap == GetUnencryptedBlockMask(response));

            TVector<TString> blocks;
            auto sglist = ResizeBlocks(blocks, range.Size(), emptyBlock);
            auto localResponse = partition.ReadBlocksLocal(range, sglist);
            UNIT_ASSERT(bitmap == GetUnencryptedBlockMask(localResponse));
        }

        {
            TBlockRange32 zeroRange(1, 3);
            partition.ZeroBlocks(zeroRange);
            MarkZeroedBlocks(bitmap, zeroRange);

            TBlockRange32 writeRange(5, 7);
            partition.WriteBlocks(writeRange, char(4));
            MarkWrittenBlocks(bitmap, writeRange);

            TBlockRange32 zeroRangeLocal(8, 10);
            partition.ZeroBlocks(zeroRangeLocal);
            MarkZeroedBlocks(bitmap, zeroRangeLocal);

            TBlockRange32 writeRangeLocal(12, 14);
            partition.WriteBlocksLocal(writeRangeLocal, GetBlockContent(4));
            MarkWrittenBlocks(bitmap, writeRangeLocal);

            partition.Flush();

            auto response = partition.ReadBlocks(range);
            UNIT_ASSERT(bitmap == GetUnencryptedBlockMask(response));

            TVector<TString> blocks;
            auto sglist = ResizeBlocks(blocks, range.Size(), emptyBlock);
            auto localResponse = partition.ReadBlocksLocal(range, sglist);
            UNIT_ASSERT(bitmap == GetUnencryptedBlockMask(localResponse));
        }
    }

    Y_UNIT_TEST(ShouldFillEnryptedBlockMaskWhenReadBlockFromOverlayDisk)
    {
        TPartitionContent baseContent = {
        /*|    0     |    1    |    2     |    3 ... 5    |    6     |    7    |    8    |    9     |    10...12    |    13    |   14    |    15    |*/
            TFresh(1), TEmpty(), TFresh(2), TBlob(2, 3, 3), TFresh(4), TEmpty(), TEmpty(), TFresh(5), TBlob(3, 6, 3), TFresh(7), TEmpty(), TFresh(8)
        };

        TBlockRange32 range(0, 15);
        auto emptyBlock = TString::Uninitialized(DefaultBlockSize);

        auto bitmap = CreateBitmap(16);

        auto partitionWithRuntime =
            SetupOverlayPartition(TestTabletId, TestTabletId2, baseContent);
        auto& partition = *partitionWithRuntime.Partition;

        {
            partition.Flush();

            auto response = partition.ReadBlocks(range);
            UNIT_ASSERT(bitmap == GetUnencryptedBlockMask(response));

            TVector<TString> blocks;
            auto sglist = ResizeBlocks(blocks, range.Size(), emptyBlock);
            auto localResponse = partition.ReadBlocksLocal(range, sglist);
            UNIT_ASSERT(bitmap == GetUnencryptedBlockMask(localResponse));
        }

        {
            TBlockRange32 writeRange(1, 3);
            partition.WriteBlocks(writeRange, char(4));
            MarkWrittenBlocks(bitmap, writeRange);

            TBlockRange32 zeroRange(5, 7);
            partition.ZeroBlocks(zeroRange);
            MarkZeroedBlocks(bitmap, zeroRange);

            TBlockRange32 writeRangeLocal(8, 10);
            partition.WriteBlocksLocal(writeRangeLocal, GetBlockContent(4));
            MarkWrittenBlocks(bitmap, writeRangeLocal);

            TBlockRange32 zeroRangeLocal(12, 14);
            partition.ZeroBlocks(zeroRangeLocal);
            MarkZeroedBlocks(bitmap, zeroRangeLocal);

            partition.Flush();

            auto response = partition.ReadBlocks(range);
            UNIT_ASSERT(bitmap == GetUnencryptedBlockMask(response));

            TVector<TString> blocks;
            auto sglist = ResizeBlocks(blocks, range.Size(), emptyBlock);
            auto localResponse = partition.ReadBlocksLocal(range, sglist);
            UNIT_ASSERT(bitmap == GetUnencryptedBlockMask(localResponse));
        }

        {
            TBlockRange32 zeroRange(1, 3);
            partition.ZeroBlocks(zeroRange);
            MarkZeroedBlocks(bitmap, zeroRange);

            TBlockRange32 writeRange(5, 7);
            partition.WriteBlocks(writeRange, char(4));
            MarkWrittenBlocks(bitmap, writeRange);

            TBlockRange32 zeroRangeLocal(8, 10);
            partition.ZeroBlocks(zeroRangeLocal);
            MarkZeroedBlocks(bitmap, zeroRangeLocal);

            TBlockRange32 writeRangeLocal(12, 14);
            partition.WriteBlocksLocal(writeRangeLocal, GetBlockContent(4));
            MarkWrittenBlocks(bitmap, writeRangeLocal);

            partition.Flush();

            auto response = partition.ReadBlocks(range);
            UNIT_ASSERT(bitmap == GetUnencryptedBlockMask(response));

            TVector<TString> blocks;
            auto sglist = ResizeBlocks(blocks, range.Size(), emptyBlock);
            auto localResponse = partition.ReadBlocksLocal(range, sglist);
            UNIT_ASSERT(bitmap == GetUnencryptedBlockMask(localResponse));
        }
    }

 Y_UNIT_TEST(ShouldHandleCorruptedFreshBlobOnInitFreshBlocks)
    {
        auto config = DefaultConfig();
        config.SetFreshChannelCount(1);
        config.SetFreshChannelWriteRequestsEnabled(true);

        auto runtime = PrepareTestActorRuntime(config);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(TBlockRange32(0), 1);
        partition.WriteBlocks(TBlockRange32(1), 2);
        partition.WriteBlocks(TBlockRange32(2), 3);

        bool evRangeResultSeen = false;

        ui32 evLoadFreshBlobsCompletedCount = 0;

        runtime->SetEventFilter(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event)
            {
                switch (event->GetTypeRewrite()) {
                    case TEvBlobStorage::EvRangeResult: {
                        using TEvent = TEvBlobStorage::TEvRangeResult;
                        auto* msg = event->Get<TEvent>();

                        if (msg->From.Channel() != 4 || evRangeResultSeen) {
                            return false;
                        }

                        evRangeResultSeen = true;

                        auto response = std::make_unique<TEvent>(
                            msg->Status,
                            msg->From,
                            msg->To,
                            msg->GroupId);

                        response->Responses = std::move(msg->Responses);

                        // corrupt
                        auto& buffer = response->Responses[0].Buffer;
                        std::memset(const_cast<char*>(buffer.data()), 0, 4);

                        auto* handle = new IEventHandle(
                            event->Recipient,
                            event->Sender,
                            response.release(),
                            0,
                            event->Cookie);

                        runtime.Send(handle, 0);

                        return true;
                    }
                    case TEvPartitionCommonPrivate::EvLoadFreshBlobsCompleted: {
                        ++evLoadFreshBlobsCompletedCount;
                        return false;
                    }
                    default: {
                        return false;
                    }
                }
            }
        );

        partition.RebootTablet();
        partition.WaitReady();

        UNIT_ASSERT_VALUES_EQUAL(true, evRangeResultSeen);

        // tablet rebooted twice (after explicit RebootTablet() and on fail)
        UNIT_ASSERT_VALUES_EQUAL(2, evLoadFreshBlobsCompletedCount);

        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(1),
            GetBlockContent(partition.ReadBlocks(0)));
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(2),
            GetBlockContent(partition.ReadBlocks(1)));
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(3),
            GetBlockContent(partition.ReadBlocks(2)));
    }

    Y_UNIT_TEST(ShouldUpdateUsedBlocksMapWhenFlushingBlocksFromFreshChannel)
    {
        auto config = DefaultConfig();
        config.SetFreshChannelCount(1);
        config.SetFreshChannelWriteRequestsEnabled(true);

        auto runtime = PrepareTestActorRuntime(config);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(1);
        partition.WriteBlocks(2);
        partition.WriteBlocks(3);

        partition.ZeroBlocks(2);

        {
            auto response = partition.StatPartition();
            auto stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(0, stats.GetUsedBlocksCount());
        }

        partition.Flush();

        {
            auto response = partition.StatPartition();
            auto stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(2, stats.GetUsedBlocksCount());
        }
    }

    Y_UNIT_TEST(ShouldCorrectlyCalculateBlocksCount)
    {
        constexpr ui32 blockCount = 1024 * 1024;
        auto runtime = PrepareTestActorRuntime(DefaultConfig(), blockCount);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(TBlockRange32(0, 1024 * 10), 1);
        partition.WriteBlocks(TBlockRange32(1024 * 5, 1024 * 11), 1);

        const auto step = 16;
        for (ui32 i = 1024 * 10; i < 1024 * 12; i += step) {
            partition.WriteBlocks(TBlockRange32(i, i + step - 1), 1);
        }

        for (ui32 i = 1024 * 20; i < 1024 * 21; i += step) {
            partition.WriteBlocks(TBlockRange32(i, i + step), 1);
        }

        partition.WriteBlocks(TBlockRange32(1001111, 1001210), 1);

        partition.ZeroBlocks(TBlockRange32(1024, 3023));
        partition.ZeroBlocks(TBlockRange32(5024, 5033));

        const auto expected = 1024 * 12 + 1024 + 1 + 100 - 2000 - 10;

        /*
        partition.WriteBlocks(TBlockRange32(5024, 5043), 1);
        partition.ZeroBlocks(TBlockRange32(5024, 5033));
        const auto expected = 10;
        */

        auto response = partition.StatPartition();
        auto stats = response->Record.GetStats();
        UNIT_ASSERT_VALUES_EQUAL(expected, stats.GetUsedBlocksCount());

        partition.RebootTablet();

        response = partition.StatPartition();
        stats = response->Record.GetStats();
        UNIT_ASSERT_VALUES_EQUAL(expected, stats.GetUsedBlocksCount());

        ui32 completionStatus = -1;
        runtime->SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvPartitionPrivate::EvMetadataRebuildCompleted: {
                        using TEv =
                            TEvPartitionPrivate::TEvMetadataRebuildCompleted;
                        auto* msg = event->Get<TEv>();
                        completionStatus = msg->GetStatus();
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            }
        );

        const auto rangesPerBatch = 100;
        partition.RebuildMetadata(NProto::ERebuildMetadataType::USED_BLOCKS, rangesPerBatch);

        UNIT_ASSERT_VALUES_EQUAL(S_OK, completionStatus);

        response = partition.StatPartition();
        stats = response->Record.GetStats();
        UNIT_ASSERT_VALUES_EQUAL(expected, stats.GetUsedBlocksCount());

        partition.RebootTablet();

        response = partition.StatPartition();
        stats = response->Record.GetStats();
        UNIT_ASSERT_VALUES_EQUAL(expected, stats.GetUsedBlocksCount());
    }

    Y_UNIT_TEST(ShouldPostponeBlockCountCalculationUntilInflightWriteRequestsAreCompleted)
    {
        constexpr ui32 blockCount = 1024 * 1024;
        auto runtime = PrepareTestActorRuntime(DefaultConfig(), blockCount);

        ui64 writeCommitId = 0;

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        runtime->SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) mutable {
                switch (event->GetTypeRewrite()) {
                    case TEvPartitionPrivate::EvAddBlobsRequest: {
                        writeCommitId =
                            event->Get<TEvPartitionPrivate::TEvAddBlobsRequest>()->CommitId;
                        return TTestActorRuntime::EEventAction::DROP;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            }
        );

        partition.SendWriteBlocksRequest(TBlockRange32(0, 1024 * 10 - 1), 1);
        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));

        partition.SendMetadataRebuildBlockCountRequest(
            MakePartialBlobId(writeCommitId + 1, 0),
            100,
            MakePartialBlobId(writeCommitId + 1, 0));

        auto response = partition.RecvMetadataRebuildBlockCountResponse();
        UNIT_ASSERT(FAILED(response->GetStatus()));
    }

    Y_UNIT_TEST(ShouldRebuildBlockCountSensors)
    {
        constexpr ui32 blockCount = 1024 * 1024;
        auto runtime = PrepareTestActorRuntime(DefaultConfig(), blockCount);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(TBlockRange32(0, 1024 * 10), 1);

        auto response = partition.RebuildMetadata(NProto::ERebuildMetadataType::BLOCK_COUNT, 100);

        partition.WriteBlocks(TBlockRange32(0, 1023), 1);

        auto stats = partition.StatPartition()->Record.GetStats();
        UNIT_ASSERT_VALUES_EQUAL(1024 * 11 + 1, stats.GetMergedBlocksCount());
    }

    Y_UNIT_TEST(ShouldFailGetMetadataRebuildStatusIfNoOperationRunning)
    {
        constexpr ui32 blockCount = 1024 * 1024;
        auto runtime = PrepareTestActorRuntime(DefaultConfig(), blockCount);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(TBlockRange32(0, 1024 * 10), 1);

        partition.SendGetRebuildMetadataStatusRequest();
        auto progress = partition.RecvGetRebuildMetadataStatusResponse();
        UNIT_ASSERT(FAILED(progress->GetStatus()));
    }

    Y_UNIT_TEST(ShouldSuccessfullyRunMetadataRebuildOnEmptyDisk)
    {
        constexpr ui32 blockCount = 1024 * 1024;
        auto runtime = PrepareTestActorRuntime(DefaultConfig(), blockCount);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.RebuildMetadata(NProto::ERebuildMetadataType::BLOCK_COUNT, 10);
        TDispatchOptions options;
        options.FinalEvents.emplace_back(TEvPartitionPrivate::EvMetadataRebuildCompleted);
        runtime->DispatchEvents(options, TDuration::Seconds(1));

        auto progress = partition.GetRebuildMetadataStatus();
        UNIT_ASSERT_VALUES_EQUAL(
            0,
            progress->Record.GetProgress().GetProcessed()
        );
        UNIT_ASSERT_VALUES_EQUAL(
            0,
            progress->Record.GetProgress().GetTotal()
        );
        UNIT_ASSERT_VALUES_EQUAL(
            true,
            progress->Record.GetProgress().GetIsCompleted()
        );
    }

    Y_UNIT_TEST(ShouldReturnRebuildMetadataProgressDuringExecution)
    {
        constexpr ui32 blockCount = 1024 * 1024;
        auto runtime = PrepareTestActorRuntime(DefaultConfig(), blockCount);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(TBlockRange32(0, 1024 * 10 - 1), 1);
        partition.WriteBlocks(TBlockRange32(1024 * 10, 1024 * 20 - 1), 1);

        runtime->SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) mutable {
                switch (event->GetTypeRewrite()) {
                    case TEvPartitionPrivate::EvMetadataRebuildBlockCountResponse: {
                        const auto* msg =
                            event->Get<TEvPartitionPrivate::TEvMetadataRebuildBlockCountResponse>();
                        UNIT_ASSERT_VALUES_UNEQUAL(
                            0,
                            msg->RebuildState.MixedBlocks + msg->RebuildState.MergedBlocks);
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            }
        );

        auto response = partition.RebuildMetadata(NProto::ERebuildMetadataType::BLOCK_COUNT, 10);

        auto progress = partition.GetRebuildMetadataStatus();
        UNIT_ASSERT_VALUES_EQUAL(
            20,
            progress->Record.GetProgress().GetProcessed()
        );
        UNIT_ASSERT_VALUES_EQUAL(
            true,
            progress->Record.GetProgress().GetIsCompleted()
        );
    }

    Y_UNIT_TEST(ShouldBlockCleanupDuringMetadataRebuild)
    {
        constexpr ui32 blockCount = 1024 * 1024;
        auto runtime = PrepareTestActorRuntime(DefaultConfig(), blockCount);

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        partition.WriteBlocks(TBlockRange32(0, 1024 - 1), 1);
        partition.WriteBlocks(TBlockRange32(0, 1024 - 1), 1);
        partition.WriteBlocks(TBlockRange32(0, 1024 - 1), 1);
        partition.WriteBlocks(TBlockRange32(0, 1024 - 1), 1);

        ui32 cnt = 0;

        TAutoPtr<IEventHandle> savedEvent;

        runtime->SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) mutable {
                switch (event->GetTypeRewrite()) {
                    case TEvPartitionPrivate::EvMetadataRebuildBlockCountRequest: {
                        if (++cnt == 1) {
                            savedEvent = event.Release();
                            return TTestActorRuntime::EEventAction::DROP;
                        }
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            }
        );

        auto response = partition.RebuildMetadata(NProto::ERebuildMetadataType::BLOCK_COUNT, 10);
        partition.Compaction();

        {
            auto stats = partition.StatPartition()->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(5, stats.GetMergedBlobsCount());
        }

        {
            partition.SendCleanupRequest();
            auto response = partition.RecvCleanupResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_TRY_AGAIN, response->GetStatus());
        }

        runtime->Send(savedEvent.Release());

        TDispatchOptions options;
        options.FinalEvents.emplace_back(
            TEvPartitionPrivate::EvMetadataRebuildCompleted);
        runtime->DispatchEvents(options);

        partition.Cleanup();

        {
            auto stats = partition.StatPartition()->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(1, stats.GetMergedBlobsCount());
        }

        auto progress = partition.GetRebuildMetadataStatus();
        UNIT_ASSERT_VALUES_EQUAL(
            4,
            progress->Record.GetProgress().GetProcessed()
        );
        UNIT_ASSERT_VALUES_EQUAL(
            4,
            progress->Record.GetProgress().GetTotal()
        );
        UNIT_ASSERT_VALUES_EQUAL(
            true,
            progress->Record.GetProgress().GetIsCompleted()
        );
    }

    Y_UNIT_TEST(ShouldNotKillTabletBeforeMaxReadBlobErrorsHappen)
    {
        auto config = DefaultConfig();
        config.SetWriteBlobThreshold(1_MB);
        config.SetMaxReadBlobErrorsBeforeSuicide(5);

        auto r = PrepareTestActorRuntime(config);
        auto& runtime = *r;

        TPartitionClient partition(runtime);
        partition.WaitReady();
        partition.WriteBlocks(TBlockRange32(0, 1000), 1);

        NKikimrProto::TLogoBlobID blobId;
        {
            auto response = partition.DescribeBlocks(TBlockRange32(0, 1000), "");
            UNIT_ASSERT_VALUES_EQUAL(1, response->Record.BlobPiecesSize());
            blobId = response->Record.GetBlobPieces(0).GetBlobId();
        }

        ui32 readBlobCount = 0;
        bool readBlobShouldFail = true;

        const auto eventHandler = [&] (const TEvBlobStorage::TEvGet::TPtr& ev) {
            auto& msg = *ev->Get();

            for (ui32 i = 0; i < msg.QuerySize; i++) {
                NKikimr::TLogoBlobID expected = LogoBlobIDFromLogoBlobID(blobId);
                NKikimr::TLogoBlobID actual = msg.Queries[i].Id;

                if (expected.IsSameBlob(actual)) {
                    readBlobCount++;

                    if (readBlobShouldFail) {
                        auto response = std::make_unique<TEvBlobStorage::TEvGetResult>(
                            NKikimrProto::ERROR,
                            msg.QuerySize,
                            0);  // groupId

                        runtime.Schedule(
                            new IEventHandle(
                                ev->Sender,
                                ev->Recipient,
                                response.release(),
                                0,
                                ev->Cookie),
                            TDuration());
                        return true;
                    }
                }
            }

            return false;
        };

        runtime.SetEventFilter(
            [eventHandler] (TTestActorRuntimeBase&, TAutoPtr<IEventHandle>& ev) {
                bool handled = false;

                const auto wrapped = [&] (const auto& ev) {
                    handled = eventHandler(ev);
                };

                switch (ev->GetTypeRewrite()) {
                    hFunc(TEvBlobStorage::TEvGet, wrapped);
                }
                return handled;
            }
        );

        bool suicideHappened = false;

        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& ev) {
                switch (ev->GetTypeRewrite()) {
                    case TEvTablet::EEv::EvTabletDead: {
                        suicideHappened = true;
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, ev);
            }
        );

        for (ui32 i = 1; i < config.GetMaxReadBlobErrorsBeforeSuicide(); i++) {
            partition.SendReadBlocksRequest(0);
            auto response = partition.RecvReadBlocksResponse();
            UNIT_ASSERT(FAILED(response->GetStatus()));

            UNIT_ASSERT_VALUES_EQUAL(i, readBlobCount);
            UNIT_ASSERT(!suicideHappened);
        }

        partition.SendReadBlocksRequest(0);
        auto response = partition.RecvReadBlocksResponse();
        UNIT_ASSERT(FAILED(response->GetStatus()));

        UNIT_ASSERT_VALUES_EQUAL(
            config.GetMaxReadBlobErrorsBeforeSuicide(),
            readBlobCount);
        UNIT_ASSERT(suicideHappened);
        suicideHappened = false;

        {
            TPartitionClient partition(runtime);
            partition.WaitReady();

            readBlobShouldFail = false;
            UNIT_ASSERT_VALUES_EQUAL(
                GetBlockContent(1),
                GetBlockContent(partition.ReadBlocks(0))
            );
            UNIT_ASSERT(!suicideHappened);
        }
    }

    Y_UNIT_TEST(ShouldProcessMultipleRangesUponCompaction)
    {
        auto config = DefaultConfig();
        config.SetWriteBlobThreshold(1_MB);
        config.SetBatchCompactionEnabled(true);
        config.SetCompactionRangeCountPerRun(3);
        config.SetCompactionThreshold(999);
        config.SetCompactionGarbageThreshold(999);
        config.SetCompactionRangeGarbageThreshold(999);
        auto runtime = PrepareTestActorRuntime(
            config,
            MaxPartitionBlocksCount
        );

        TPartitionClient partition(*runtime);
        partition.WaitReady();

        const auto blockRange1 = TBlockRange32::WithLength(0, 1024);
        const auto blockRange2 = TBlockRange32::WithLength(1024 * 1024, 1024);
        const auto blockRange3 = TBlockRange32::WithLength(2 * 1024 * 1024, 1024);
        const auto blockRange4 = TBlockRange32::WithLength(3 * 1024 * 1024, 1024);

        partition.WriteBlocks(blockRange1, 1);
        partition.WriteBlocks(blockRange1, 2);
        partition.WriteBlocks(blockRange1, 3);

        partition.WriteBlocks(blockRange2, 4);
        partition.WriteBlocks(blockRange2, 5);
        partition.WriteBlocks(blockRange2, 6);

        partition.WriteBlocks(blockRange3, 7);
        partition.WriteBlocks(blockRange3, 8);
        partition.WriteBlocks(blockRange3, 9);

        partition.WriteBlocks(blockRange4, 10);
        partition.WriteBlocks(blockRange4, 11);

        // blockRange4 should not be compacted, other ranges - should
        partition.Compaction();
        partition.Cleanup();

        // checking that data wasn't corrupted
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(3),
            GetBlockContent(partition.ReadBlocks(blockRange1.Start))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(3),
            GetBlockContent(partition.ReadBlocks(blockRange1.End))
        );

        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(6),
            GetBlockContent(partition.ReadBlocks(blockRange2.Start))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(6),
            GetBlockContent(partition.ReadBlocks(blockRange2.End))
        );

        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(9),
            GetBlockContent(partition.ReadBlocks(blockRange3.Start))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(9),
            GetBlockContent(partition.ReadBlocks(blockRange3.End))
        );

        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(11),
            GetBlockContent(partition.ReadBlocks(blockRange4.Start))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            GetBlockContent(11),
            GetBlockContent(partition.ReadBlocks(blockRange4.End))
        );

        // checking that we now have 1 blob in each of the first 3 ranges
        // and 2 blobs in the last range
        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(5, stats.GetMergedBlobsCount());
        }

        // blockRange4 and any other 2 ranges should be compacted
        partition.Compaction();
        partition.Cleanup();

        // all ranges should contain 1 blob now
        {
            auto response = partition.StatPartition();
            const auto& stats = response->Record.GetStats();
            UNIT_ASSERT_VALUES_EQUAL(4, stats.GetMergedBlobsCount());
        }
    }
}

}   // namespace NCloud::NBlockStore::NStorage::NPartition

