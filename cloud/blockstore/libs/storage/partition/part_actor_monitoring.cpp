#include "part_actor.h"

#include <cloud/blockstore/libs/diagnostics/config.h>
#include <cloud/blockstore/libs/diagnostics/hostname.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/monitoring_utils.h>
#include <cloud/blockstore/libs/storage/core/probes.h>
#include <cloud/blockstore/libs/storage/model/channel_data_kind.h>
#include <cloud/storage/core/libs/common/format.h>

#include <ydb/core/base/appdata.h>

#include <library/cpp/monlib/service/pages/templates.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <util/stream/str.h>

namespace NCloud::NBlockStore::NStorage::NPartition {

using namespace NKikimr;

using namespace NMonitoringUtils;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER);

namespace {

////////////////////////////////////////////////////////////////////////////////

ui64 GetHiveTabletId(const TActorContext& ctx)
{
    auto& domainsInfo = *AppData(ctx)->DomainsInfo;
    Y_VERIFY(domainsInfo.Domains);
    auto domainUid = domainsInfo.Domains.begin()->first;
    auto hiveUid = domainsInfo.GetDefaultHiveUid(domainUid);
    return domainsInfo.GetHive(hiveUid);
}

void DumpChannels(
    IOutputStream& out,
    const TPartitionState& state,
    const TTabletStorageInfo& storage,
    const TDiagnosticsConfig& config,
    ui64 hiveTabletId)
{
    HTML(out) {
        DIV() {
            out << "<p><a href='app?TabletID=" << hiveTabletId
                << "&page=Groups"
                << "&tablet_id=" << storage.TabletID
                << "'>Channel history</a></p>";
        }

        TABLE_CLASS("table table-condensed") {
            TABLEBODY() {
                for (const auto& channel: storage.Channels) {
                    TABLER() {
                        TABLED() { out << "Channel: " << channel.Channel; }
                        TABLED() { out << "StoragePool: " << channel.StoragePool; }

                        if (auto latestEntry = channel.LatestEntry()) {
                            TABLED() { out << "Id: " << latestEntry->GroupID; }
                            TABLED() { out << "Gen: " << latestEntry->FromGeneration; }
                            const auto& cps =
                                state.GetConfig().GetExplicitChannelProfiles();
                            if (cps.size()) {
                                // we need this check for legacy volumes
                                // see NBS-752
                                if (channel.Channel < static_cast<ui32>(cps.size())) {
                                    const auto dataKind = static_cast<EChannelDataKind>(
                                        cps[channel.Channel].GetDataKind());
                                    const auto& poolKind =
                                        cps[channel.Channel].GetPoolKind();
                                    TABLED() { out << "PoolKind: " << poolKind; }
                                    TABLED() { out << "DataKind: " << dataKind; }
                                } else {
                                    // we need to output 2 cells, otherwise table
                                    // markup will be a bit broken
                                    TABLED() { out << "Ghost"; }
                                    TABLED() { out << "Channel"; }
                                }
                            }
                            TABLED() {
                                TStringBuf label;
                                TStringBuf color;
                                if (state.CheckPermissions(channel.Channel, EChannelPermission::SystemWritesAllowed)) {
                                    if (state.CheckPermissions(channel.Channel, EChannelPermission::UserWritesAllowed)) {
                                        color = "green";
                                        label = "Writable";
                                    } else {
                                        color = "yellow";
                                        label = "SystemWritable";
                                    }
                                } else {
                                    if (state.CheckPermissions(channel.Channel, EChannelPermission::UserWritesAllowed)) {
                                        color = "pink";
                                        label = "WeirdState";
                                    } else {
                                        color = "orange";
                                        label = "Readonly";
                                    }
                                }

                                SPAN_CLASS_STYLE("label", TStringBuilder() << "background-color: " << color) {
                                    out << label;
                                }
                            }
                            TABLED() {
                                out << "<a href='"
                                    << "../actors/blobstorageproxies/blobstorageproxy"
                                    << latestEntry->GroupID
                                    << "'>Status</a>";
                            }
                            TABLED() {
                                out << "<a href='"
                                    << GetSolomonBsProxyUrl(
                                           config,
                                           latestEntry->GroupID,
                                           "nbs-dsproxy-percentile")
                                    << "'>Graphs</a>";
                            }
                            TABLED() {
                                BuildReassignChannelButton(
                                    out,
                                    hiveTabletId,
                                    storage.TabletID,
                                    channel.Channel);
                            }
                        }
                    }
                }
            }
        }
    }
}

void DumpCheckpoints(
    IOutputStream& out,
    const TTabletStorageInfo& storage,
    ui32 freshBlocksCount,
    ui32 blockSize,
    const TVector<TCheckpoint>& checkpoints,
    const THashMap<TString, ui64>& checkpointId2CommitId)
{
    Y_UNUSED(storage);

    HTML(out) {
        TABLE_SORTABLE() {
            TABLEHEAD() {
                TABLER() {
                    TABLED() { out << "CheckpointId"; }
                    TABLED() { out << "CommitId"; }
                    TABLED() { out << "IdempotenceId"; }
                    TABLED() { out << "Time"; }
                    TABLED() { out << "Size"; }
                    TABLED() { out << "DataDeleted"; }
                }
            }
            TABLEBODY() {
                for (const auto& mapping: checkpointId2CommitId) {
                    const auto& checkpointId = mapping.first;
                    const auto& commitId = mapping.second;

                    const auto* checkpoint = FindIfPtr(
                        checkpoints,
                        [&](const auto& ckp) {
                            return ckp.CheckpointId == checkpointId;
                    });

                    TABLER() {
                        TABLED() { out << checkpointId; }
                        TABLED() { out << commitId; }
                        TABLED() { out << (checkpoint ? checkpoint->IdempotenceId : ""); }
                        TABLED() { out << FormatTimestamp(checkpoint ? checkpoint->DateCreated : TInstant::Zero()); }
                        TABLED() {
                            ui64 byteSize = 0;
                            if (checkpoint) {
                                auto blocksCount = freshBlocksCount;
                                blocksCount += checkpoint->Stats.GetMixedBlocksCount();
                                blocksCount += checkpoint->Stats.GetMergedBlocksCount();
                                byteSize = static_cast<ui64>(blocksCount) * blockSize;
                            }

                            out << FormatByteSize(byteSize);
                        }
                        TABLED() { out << (checkpoint ? "" : "true"); }
                    }
                }
            }
        }
    }
}

void DumpCleanupQueue(
    IOutputStream& out,
    const TTabletStorageInfo& storage,
    const TVector<TCleanupQueueItem>& items)
{
    HTML(out) {
        TABLE_SORTABLE() {
            TABLEHEAD() {
                TABLER() {
                    TABLED() { out << "CommitId"; }
                    TABLED() { out << "BlobId"; }
                    TABLED() { out << "Deleted"; }
                }
            }
            TABLEBODY() {
                for (const auto& item: items) {
                    TABLER() {
                        TABLED() { DumpCommitId(out, item.BlobId.CommitId()); }
                        TABLED_CLASS("view") { DumpBlobId(out, storage, item.BlobId); }
                        TABLED() { DumpCommitId(out, item.CommitId); }
                    }
                }
            }
        }
    }
}

void DumpProgress(IOutputStream& out, ui64 progress, ui64 total)
{
    HTML(out) {
        DIV_CLASS("progress") {
            ui32 percents = (progress * 100 / total);
            out << "<div class='progress-bar' role='progressbar' aria-valuemin='0'"
                << " style='width: " << percents << "%'"
                << " aria-valuenow='" << progress
                << "' aria-valuemax='" << total << "'>"
                << percents << "%</div>";
        }
        out << progress << " of " << total;
    }
}

void DumpCompactionInfo(IOutputStream& out, const TForcedCompactionState& state)
{
    if (state.IsRunning && (state.OperationId == "partition-monitoring-compaction")) {
        DumpProgress(out, state.Progress, state.RangesCount);
    }
}

void DumpMetadataRebuildInfo(IOutputStream& out, ui64 current, ui64 total)
{
    DumpProgress(out, current, total);
}

void DumpCompactionScoreHistory(
    IOutputStream& out,
    const TRingBuffer<TCompactionScores>& scoreHistory)
{
    HTML(out) {
        TABLE_SORTABLE() {
            TABLEHEAD() {
                TABLER() {
                    TABLED() { out << "Ts"; }
                    TABLED() { out << "Score"; }
                    TABLED() { out << "GarbageScore"; }
                }
            }
            TABLEBODY() {
                for (ui32 i = 0; i < scoreHistory.Size(); ++i) {
                    const auto s = scoreHistory.Get(i);

                    TABLER() {
                        TABLED() { out << s.Ts; }
                        TABLED() { out << s.Value.Score; }
                        TABLED() { out << s.Value.GarbageScore; }
                    }
                }
            }
        }
    }
}

void DumpCleanupScoreHistory(
    IOutputStream& out,
    const TRingBuffer<ui32>& scoreHistory)
{
    HTML(out) {
        TABLE_SORTABLE() {
            TABLEHEAD() {
                TABLER() {
                    TABLED() { out << "Ts"; }
                    TABLED() { out << "QueueSize"; }
                }
            }
            TABLEBODY() {
                for (ui32 i = 0; i < scoreHistory.Size(); ++i) {
                    const auto s = scoreHistory.Get(i);

                    TABLER() {
                        TABLED() { out << s.Ts; }
                        TABLED() { out << s.Value; }
                    }
                }
            }
        }
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TPartitionActor::HandleHttpInfo(
    const NMon::TEvRemoteHttpInfo::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        MakeIntrusive<TCallContext>(),
        std::move(ev->TraceId));

    BLOCKSTORE_TRACE_RECEIVED(ctx, &requestInfo->TraceId, this, msg);

    LWPROBE(
        RequestReceived_Partition,
        "HttpInfo",
        GetRequestId(requestInfo->TraceId));

    LOG_DEBUG(ctx, TBlockStoreComponents::PARTITION,
        "[%lu] HTTP request: %s",
        TabletID(),
        msg->Query.Quote().data());

    if (State && State->IsLoadStateFinished()) {
        const auto& params = msg->Cgi();

        const auto& page = params.Get("page");
        if (page == "describe") {
            return HandleHttpInfo_Describe(ctx, std::move(requestInfo), params);
        } else if (page == "check") {
            return HandleHttpInfo_Check(ctx, std::move(requestInfo), params);
        } else if (page == "view") {
            return HandleHttpInfo_View(ctx, std::move(requestInfo), params);
        } else {
            const auto& action = params.Get("action");
            // all actions should have POST method
            if (action && (ev->Get()->GetMethod() != HTTP_METHOD_POST)) {
                using namespace NMonitoringUtils;

                TString msg = "Wrong HTTP method";
                LOG_ERROR_S(ctx, TBlockStoreComponents::SERVICE, msg);

                TStringStream out;
                BuildTabletNotifyPageWithRedirect(
                    out,
                    msg,
                    TabletID(),
                    EAlertLevel::DANGER);

                auto response = std::make_unique<NMon::TEvRemoteHttpInfoRes>(out.Str());
                NCloud::Reply(ctx, *ev, std::move(response));

                return;
            }
            if (action == "compactAll" || action == "compact") {
                return HandleHttpInfo_ForceCompaction(ctx, std::move(requestInfo), params);
            } else if (action == "rebuildMetadata") {
                return HandleHttpInfo_RebuildMetadata(ctx, std::move(requestInfo), params);
            } else if (action == "addGarbage") {
                return HandleHttpInfo_AddGarbage(ctx, std::move(requestInfo), params);
            } else if (action == "collectGarbage") {
                return HandleHttpInfo_CollectGarbage(ctx, std::move(requestInfo), params);
            }
        }

        HandleHttpInfo_Default(ctx, std::move(requestInfo), params);
    } else {
        TStringStream out;

        DumpTabletNotReady(out);
        auto response = std::make_unique<NMon::TEvRemoteHttpInfoRes>(out.Str());

        BLOCKSTORE_TRACE_SENT(ctx, &requestInfo->TraceId, this, response);

        LWPROBE(
            ResponseSent_Partition,
            "HttpInfo",
            GetRequestId(requestInfo->TraceId));

        NCloud::Reply(ctx, *requestInfo, std::move(response));
    }
}

void TPartitionActor::HandleHttpInfo_Default(
    const TActorContext& ctx,
    TRequestInfoPtr requestInfo,
    const TCgiParameters& params)
{
    Y_UNUSED(params);

    TStringStream out;
    HTML(out) {
        DIV_CLASS_ID("container-fluid", "tabs") {
            BuildPartitionTabs(out);

            DIV_CLASS("tab-content") {
                DIV_CLASS_ID("tab-pane active", "Overview") {
                    DumpDefaultHeader(out, *Info(), SelfId().NodeId(), *DiagnosticsConfig);
                    DumpSolomonPartitionLink(out, *DiagnosticsConfig);

                    H3() { out << "State"; }
                    State->DumpHtml(out);

                    H3() { out << "Partition Statistics"; }
                    DumpPartitionStats(out, State->GetConfig(), State->GetStats(), State->GetUnflushedFreshBlocksCount());

                    H3() { out << "Partition Counters"; }
                    DumpPartitionCounters(out, State->GetStats());

                    H3() { out << "Partition Config"; }
                    DumpPartitionConfig(out, State->GetConfig());

                    H3() { out << "Misc"; }
                    TABLE_CLASS("table table-condensed") {
                        TABLEBODY() {
                            TABLER() {
                                TABLED() { out << "Executor Reject Probability"; }
                                TABLED() { out << Executor()->GetRejectProbability(); }
                            }
                        }
                    }
                }

                DIV_CLASS_ID("tab-pane", "Tables") {
                    H3() {
                        out << "Checkpoints";
                    }

                    DumpCheckpoints(
                        out,
                        *Info(),
                        State->GetUnflushedFreshBlocksCount(),
                        State->GetBlockSize(),
                        State->GetCheckpoints().Get(),
                        State->GetCheckpoints().GetMapping());

                    H3() {
                        if (!State->IsForcedCompactionRunning()) {
                            BuildMenuButton(out, "compact-all");
                        }
                        out << "CompactionQueue";
                    }

                    if (State->IsForcedCompactionRunning()) {
                        DumpCompactionInfo(out, State->GetForcedCompactionState());
                    } else {
                        out << "<div class='collapse form-group' id='compact-all'>";
                        BuildForceCompactionButton(out, TabletID());
                        out << "</div>";
                    }

                    H3() {
                        out << "ByScore";
                    }

                    DumpCompactionMap(
                        out,
                        *Info(),
                        State->GetCompactionMap().GetTop(10),
                        State->GetCompactionMap().GetRangeSize()
                    );

                    H3() {
                        out << "ByGarbageScore";
                    }

                    DumpCompactionMap(
                        out,
                        *Info(),
                        State->GetCompactionMap().GetTopByGarbageBlockCount(10),
                        State->GetCompactionMap().GetRangeSize()
                    );

                    H3() {
                        out << "CompactionScoreHistory";
                    }

                    DumpCompactionScoreHistory(
                        out,
                        State->GetCompactionScoreHistory()
                    );

                    H3() { out << "CleanupQueue"; }
                    DumpCleanupQueue(out, *Info(), State->GetCleanupQueue().GetItems());

                    H3() {
                        out << "CleanupScoreHistory";
                    }

                    DumpCleanupScoreHistory(
                        out,
                        State->GetCleanupScoreHistory()
                    );

                    H3() { out << "NewBlobs"; }
                    DumpBlobs(out, *Info(), State->GetGarbageQueue().GetNewBlobs());

                    H3() {
                        BuildMenuButton(out, "garbage-options");
                        out << "GarbageBlobs";
                    }

                    out << "<div class='collapse form-group' id='garbage-options'>";
                    PARA() {
                        out << "<a href='' data-toggle='modal' data-target='#collect-garbage'>Collect Garbage</a>";
                    }
                    PARA() {
                        out << "<a href='' data-toggle='modal' data-target='#set-hard-barriers'>Set Hard Barriers</a>";
                    }
                    BuildAddGarbageButton(out, TabletID());
                    BuildCollectGarbageButton(out, TabletID());
                    BuildSetHardBarriers(out, TabletID());
                    out << "</div>";

                    DumpBlobs(out, *Info(), State->GetGarbageQueue().GetGarbageBlobs());

                    H3() {
                        BuildMenuButton(out, "metadata-rebuild");
                        out << "Rebuild metadata";
                    }

                    if (State->IsMetadataRebuildStarted()) {
                        const auto progress = State->GetMetadataRebuildProgress();
                        DumpMetadataRebuildInfo(out, progress.Processed, progress.Total);
                    } else {
                        out << "<div class='collapse form-group' id='metadata-rebuild'>";
                        for (const auto rangesPerBatch : {1, 10, 100}) {
                            BuildRebuildMetadataButton(out, TabletID(), rangesPerBatch);
                        }
                        out << "</div>";
                    }
                }

                DIV_CLASS_ID("tab-pane", "Channels") {
                    H3() {
                        BuildMenuButton(out, "reassign-all");
                        out << "Channels";
                    }
                    out << "<div class='collapse form-group' id='reassign-all'>";
                    BuildReassignChannelsButton(
                        out,
                        GetHiveTabletId(ctx),
                        Info()->TabletID);
                    out << "</div>";
                    DumpChannels(
                        out,
                        *State,
                        *Info(),
                        *DiagnosticsConfig,
                        GetHiveTabletId(ctx));
                }

                DIV_CLASS_ID("tab-pane", "Index") {
                    DumpDescribeHeader(out, *Info());
                    DumpCheckHeader(out, *Info());
                }
            }
        }

        GeneratePartitionTabsJs(out);
        GenerateBlobviewJS(out);
        GenerateActionsJS(out);
    }

    auto response = std::make_unique<NMon::TEvRemoteHttpInfoRes>(out.Str());

    BLOCKSTORE_TRACE_SENT(ctx, &requestInfo->TraceId, this, response);

    LWPROBE(
        ResponseSent_Partition,
        "HttpInfo",
        GetRequestId(requestInfo->TraceId));

    NCloud::Reply(ctx, *requestInfo, std::move(response));
}

}   // namespace NCloud::NBlockStore::NStorage::NPartition
