#include "tablet_actor.h"

#include "helpers.h"

#include <cloud/filestore/libs/diagnostics/critical_events.h>
#include <cloud/filestore/libs/diagnostics/storage_counters.h>

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

namespace {

////////////////////////////////////////////////////////////////////////////////

NProto::TTabletStorageInfo SerializeTabletStorageInfo(const auto& info)
{
    NProto::TTabletStorageInfo proto;

    proto.SetTabletId(info.TabletID);
    proto.SetVersion(info.Version);

    for (const auto& srcChannel: info.Channels) {
        auto& dstChannel = *proto.MutableChannels()->Add();
        dstChannel.SetStoragePool(srcChannel.StoragePool);

        for (const auto& srcEntry: srcChannel.History) {
            auto& dstEntry = *dstChannel.MutableHistory()->Add();
            dstEntry.SetFromGeneration(srcEntry.FromGeneration);
            dstEntry.SetGroupId(srcEntry.GroupID);
        }
    }

    return proto;
}

TString ValidateTabletStorageInfoUpdate(
    const NProto::TTabletStorageInfo& oldInfo,
    const NProto::TTabletStorageInfo& newInfo)
{
    const ui32 oldInfoVersion = oldInfo.GetVersion();
    const ui32 newInfoVersion = newInfo.GetVersion();

    if (oldInfoVersion > newInfoVersion) {
        return TStringBuilder()
            << "version mismatch (old: " << oldInfoVersion
            << ", new: " << newInfoVersion << ")";
    }

    if (oldInfoVersion == newInfoVersion) {
        google::protobuf::util::MessageDifferencer differencer;
        TString diff;
        differencer.ReportDifferencesToString(&diff);

        if (differencer.Compare(oldInfo, newInfo)) {
            // Nothing has changed.
            return {};
        }

        return TStringBuilder()
            << "content has changed without version increment, diff: " << diff;
    }

    Y_VERIFY(oldInfoVersion < newInfoVersion);

    const ui32 oldChannelCount = oldInfo.ChannelsSize();
    const ui32 newChannelCount = newInfo.ChannelsSize();;

    if (oldChannelCount > newChannelCount) {
        return TStringBuilder()
            << "channel count has changed (old: " << oldChannelCount
            << ", new: " << newChannelCount << ")";
    }

    return {};
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

bool TIndexTabletActor::PrepareTx_LoadState(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TLoadState& args)
{
    Y_UNUSED(ctx);

    TIndexTabletDatabase db(tx.DB);

    bool ready = true;

    if (!db.ReadFileSystem(args.FileSystem)) {
        ready = false;
    }

    if (!db.ReadFileSystemStats(args.FileSystemStats)) {
        ready = false;
    }

    if (!db.ReadTabletStorageInfo(args.TabletStorageInfo)) {
        ready = false;
    }

    if (!db.ReadNode(RootNodeId, 0, args.RootNode)) {
        ready = false;
    }

    if (!db.ReadSessions(args.Sessions)) {
        ready = false;
    }

    if (!db.ReadSessionHandles(args.Handles)) {
        ready = false;
    }

    if (!db.ReadSessionLocks(args.Locks)) {
        ready = false;
    }

    if (!db.ReadSessionDupCacheEntries(args.DupCache)) {
        ready = false;
    }

    if (!db.ReadFreshBytes(args.FreshBytes)) {
        ready = false;
    }

    if (!db.ReadFreshBlocks(args.FreshBlocks)) {
        ready = false;
    }

    if (!db.ReadNewBlobs(args.NewBlobs)) {
        ready = false;
    }

    if (!db.ReadGarbageBlobs(args.GarbageBlobs)) {
        ready = false;
    }

    if (!db.ReadCheckpoints(args.Checkpoints)) {
        ready = false;
    }

    if (!db.ReadCompactionMap(args.CompactionMap)) {
        ready = false;
    }

    return ready;
}

void TIndexTabletActor::ExecuteTx_LoadState(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TLoadState& args)
{
    Y_UNUSED(ctx);

    TIndexTabletDatabase db(tx.DB);

    if (!args.RootNode) {
        args.RootNode.ConstructInPlace();
        args.RootNode->Attrs = CreateDirectoryAttrs(0777, 0, 0);
        db.WriteNode(RootNodeId, 0, args.RootNode->Attrs);
    }

    const auto& oldTabletStorageInfo = args.TabletStorageInfo;
    const auto newTabletStorageInfo = SerializeTabletStorageInfo(*Info());

    if (!oldTabletStorageInfo.GetTabletId()) {
        // First TxLoadState on tablet creation
        LOG_INFO(ctx, TFileStoreComponents::TABLET,
            "[%lu] Initializing tablet storage info",
            TabletID());

        Y_VERIFY(newTabletStorageInfo.GetTabletId());
        args.TabletStorageInfo.CopyFrom(newTabletStorageInfo);
        db.WriteTabletStorageInfo(newTabletStorageInfo);
        return;
    }

    const auto error = ValidateTabletStorageInfoUpdate(
        oldTabletStorageInfo,
        newTabletStorageInfo);

    if (error) {
        ReportInvalidTabletStorageInfo();
        args.Error = MakeError(E_FAIL, TStringBuilder()
            << "failed to update TabletStorageInfo: " << error);
        return;
    }

    LOG_INFO(ctx, TFileStoreComponents::TABLET,
        "[%lu] Updating TabletStorageInfo",
        TabletID());

    args.TabletStorageInfo.CopyFrom(newTabletStorageInfo);
    db.WriteTabletStorageInfo(newTabletStorageInfo);
}

void TIndexTabletActor::CompleteTx_LoadState(
    const TActorContext& ctx,
    TTxIndexTablet::TLoadState& args)
{
    if (FAILED(args.Error.GetCode())) {
        LOG_ERROR_S(ctx, TFileStoreComponents::TABLET,
            "[" << TabletID() << "]"
            << "Switch tablet to BROKEN state due to the failed TxLoadState: "
            << args.Error.GetMessage());

        BecomeAux(ctx, STATE_BROKEN);

        // allow pipes to connect
        SignalTabletActive(ctx);

        // resend pending WaitReady requests
        while (WaitReadyRequests) {
            ctx.Send(WaitReadyRequests.front().release());
            WaitReadyRequests.pop_front();
        }

        return;
    }

    BecomeAux(ctx, STATE_WORK);

    // allow pipes to connect
    SignalTabletActive(ctx);

    // resend pending WaitReady requests
    while (WaitReadyRequests) {
        ctx.Send(WaitReadyRequests.front().release());
        WaitReadyRequests.pop_front();
    }

    LoadState(
        Executor()->Generation(),
        args.FileSystem,
        args.FileSystemStats,
        args.TabletStorageInfo);

    auto idleSessionDeadline = ctx.Now() + Config->GetIdleSessionTimeout();

    LoadSessions(
        idleSessionDeadline,
        args.Sessions,
        args.Handles,
        args.Locks,
        args.DupCache);

    if (!Config->GetEnableCollectGarbageAtStart()) {
        SetStartupGcExecuted();
    }

    // checkpoints should be loaded before data
    LoadCheckpoints(args.Checkpoints);
    LoadFreshBytes(args.FreshBytes);
    LoadFreshBlocks(args.FreshBlocks);
    LoadGarbage(args.NewBlobs, args.GarbageBlobs);
    LoadCompactionMap(args.CompactionMap);

    ScheduleCleanupSessions(ctx);
    RestartCheckpointDestruction(ctx);
    EnqueueFlushIfNeeded(ctx);
    EnqueueBlobIndexOpIfNeeded(ctx);
    EnqueueCollectGarbageIfNeeded(ctx);

    RegisterStatCounters();
}

}   // namespace NCloud::NFileStore::NStorage
