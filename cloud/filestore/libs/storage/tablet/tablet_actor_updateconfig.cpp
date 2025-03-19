#include "tablet_actor.h"

#include <cloud/filestore/libs/diagnostics/critical_events.h>

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

namespace {

////////////////////////////////////////////////////////////////////////////////

TString ValidateUpdateConfigRequest(
    const NProto::TFileSystem& oldConfig,
    const NProto::TFileSystem& newConfig)
{
    const ui32 oldBlockSize = oldConfig.GetBlockSize();
    const ui32 newBlockSize = newConfig.GetBlockSize();

    if (oldBlockSize != newBlockSize) {
        return TStringBuilder()
            << "it's not allowed to change blockSize"
            << " (old: " << oldBlockSize
            << ", new: " << newBlockSize
            << ")";
    }

    const ui64 oldBlockCount = oldConfig.GetBlocksCount();
    const ui64 newBlockCount = newConfig.GetBlocksCount();

    if (oldBlockCount > newBlockCount) {
        return TStringBuilder()
            << "it's not allowed to decrease blockCount"
            << " (old: " << oldBlockCount
            << ", new: " << newBlockCount
            << ")";
    }

    const ui32 oldChannelCount = oldConfig.ExplicitChannelProfilesSize();
    const ui32 newChannelCount = newConfig.ExplicitChannelProfilesSize();

    if (oldChannelCount > newChannelCount) {
        return TStringBuilder()
            << "it's not allowed to decrease channelCount"
            << " (old: " << oldChannelCount
            << ", new: " << newChannelCount
            << ")";
    }

    using TChannelDiff = std::tuple<ui32, EChannelDataKind, EChannelDataKind>;
    TVector<TChannelDiff> ChangedChannels;

    for (ui32 channel = 0; channel < oldChannelCount; ++channel) {
        const auto oldDataKind = static_cast<EChannelDataKind>(oldConfig
            .GetExplicitChannelProfiles(channel)
            .GetDataKind());

        const auto newDataKind = static_cast<EChannelDataKind>(newConfig
            .GetExplicitChannelProfiles(channel)
            .GetDataKind());

        if (oldDataKind != newDataKind) {
            ChangedChannels.emplace_back(channel, oldDataKind, newDataKind);
        }
    }

    if (ChangedChannels) {
        auto error = TStringBuilder()
            << "it's not allowed to change dataKind of existing channels [";

        for (const auto& [channel, oldDataKind, newDataKind]: ChangedChannels) {
            error << " (channel: " << channel
                << ", oldDataKind: " << ToString(oldDataKind)
                << ", newDataKind: " << ToString(newDataKind)
                << ") ";
        }

        error << "]";
        return error;
    }

    if (oldChannelCount > 0) {
        // Resizing tablet: check new channels dataKind.

        using TChannelDesc = std::tuple<ui32, EChannelDataKind>;
        TVector<TChannelDesc> BadNewChannels;

        for (ui32 channel = oldChannelCount; channel < newChannelCount; ++channel) {
            const auto dataKind = static_cast<EChannelDataKind>(newConfig
                .GetExplicitChannelProfiles(channel)
                .GetDataKind());

            if (dataKind != EChannelDataKind::Mixed) {
                BadNewChannels.emplace_back(channel, dataKind);
            }
        }

        if (BadNewChannels) {
            auto error = TStringBuilder()
                << "it's allowed to add new channels with Mixed dataKind only [";

            for (const auto& [channel, dataKind]: BadNewChannels) {
                error << " (channel: " << channel
                    << ", dataKind: " << ToString(dataKind)
                    << ") ";
            }

            error << "]";
            return error;
        }
    }

    return {};
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TIndexTabletActor::HandleUpdateConfig(
    const TEvFileStore::TEvUpdateConfig::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        MakeIntrusive<TCallContext>(),  // external event
        std::move(ev->TraceId));

    const ui64 txId = msg->Record.GetTxId();

    const auto& oldConfig = GetFileSystem();
    const auto newConfig = CopyFrom(msg->Record.GetConfig());

    if (!GetFileSystemId()) {
        LOG_INFO(ctx,TFileStoreComponents::TABLET,
            "[%lu] Starting tablet config initialization [txId: %d]",
            TabletID(),
            txId);

        // First config update on tablet creation. No need to validate config.
        ExecuteTx<TUpdateConfig>(
            ctx,
            std::move(requestInfo),
            txId,
            std::move(newConfig));

        return;
    }

    // Config update occured due to alter/resize.
    if (auto error = ValidateUpdateConfigRequest(oldConfig, newConfig)) {
        LOG_ERROR(ctx, TFileStoreComponents::TABLET,
            "[%lu] Failed to update config [txId: %d]: %s",
            TabletID(),
            txId,
            error.c_str());

        ReportTabletUpdateConfigError();

        using TResponse = TEvFileStore::TEvUpdateConfigResponse;
        auto response = std::make_unique<TResponse>();

        response->Record.SetTxId(txId);
        response->Record.SetOrigin(TabletID());
        // do not return ERROR code: it makes schemeshard verify
        response->Record.SetStatus(NKikimrFileStore::OK);

        NCloud::Reply(ctx, *requestInfo, std::move(response));
        return;
    }

    ExecuteTx<TUpdateConfig>(
        ctx,
        std::move(requestInfo),
        txId,
        std::move(newConfig));
}

////////////////////////////////////////////////////////////////////////////////

bool TIndexTabletActor::PrepareTx_UpdateConfig(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TUpdateConfig& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);

    return true;
}

void TIndexTabletActor::ExecuteTx_UpdateConfig(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TUpdateConfig& args)
{
    Y_UNUSED(ctx);

    TIndexTabletDatabase db(tx.DB);

    UpdateConfig(db, args.FileSystem);
}

void TIndexTabletActor::CompleteTx_UpdateConfig(
    const TActorContext& ctx,
    TTxIndexTablet::TUpdateConfig& args)
{
    // register stat counters w proper volume information
    RegisterStatCounters();

    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] Sending OK response for UpdateConfig with version=%u",
        TabletID(),
        args.FileSystem.GetVersion());

    auto response = std::make_unique<TEvFileStore::TEvUpdateConfigResponse>();
    response->Record.SetTxId(args.TxId);
    response->Record.SetOrigin(TabletID());
    response->Record.SetStatus(NKikimrFileStore::OK);

    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
}

}   // namespace NCloud::NFileStore::NStorage
