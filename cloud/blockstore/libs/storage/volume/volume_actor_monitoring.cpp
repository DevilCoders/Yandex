#include "volume_actor.h"

#include <cloud/blockstore/libs/diagnostics/config.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/monitoring_utils.h>
#include <cloud/blockstore/libs/storage/partition_nonrepl/config.h>

#include <cloud/storage/core/libs/common/format.h>
#include <cloud/storage/core/libs/common/media.h>

#include <library/cpp/monlib/service/pages/templates.h>
#include <library/cpp/protobuf/util/pb_io.h>

#include <util/stream/str.h>
#include <util/string/join.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;
using namespace NMonitoringUtils;

namespace {

////////////////////////////////////////////////////////////////////////////////

IOutputStream& operator <<(
    IOutputStream& out,
    NProto::EVolumeIOMode ioMode)
{
    switch (ioMode) {
        case NProto::VOLUME_IO_OK:
            return out << "<font color=green>ok</font>";
        case NProto::VOLUME_IO_ERROR_READ_ONLY:
            return out << "<font color=red>read only</font>";
        default:
            return out
                << "(Unknown EVolumeIOMode value "
                << static_cast<int>(ioMode)
                << ")";
    }
}

IOutputStream& operator <<(
    IOutputStream& out,
    NProto::EVolumeAccessMode accessMode)
{
    switch (accessMode) {
        case NProto::VOLUME_ACCESS_READ_WRITE:
            return out << "Read/Write";
        case NProto::VOLUME_ACCESS_READ_ONLY:
            return out << "Read only";
        case NProto::VOLUME_ACCESS_REPAIR:
            return out << "Repair";
        case NProto::VOLUME_ACCESS_USER_READ_ONLY:
            return out << "User read only";
        default:
            Y_VERIFY_DEBUG(false, "Unknown EVolumeAccessMode: %d", accessMode);
            return out << "Undefined";
    }
}

void BuildVolumeRemoveClientButton(
    IOutputStream& out,
    ui64 id,
    const TString& diskId,
    const TString& clientId,
    const ui64 tabletId)
{
    out << "<form method='POST' name='RemoveClient_" << clientId << "'>"
        << "<input type='hidden' name='TabletID' value='" << tabletId << "'/>"
        << "<input type='hidden' name='ClientId' value='" << clientId << "'/>"
        << "<input type='hidden' name='Volume' value='" << diskId << "'/>"
        << "<input type='hidden' name='action' value='removeclient'/>"
        << "<input class='btn btn-primary' type='button' value='Remove' "
        << "data-toggle='modal' data-target='#volume-remove-client-"
        << id << "'/>"
        << "</form>" << Endl;

    BuildConfirmActionDialog(
        out,
        TStringBuilder() << "volume-remove-client-" << id,
        "Remove client",
        "Are you sure you want to remove client from volume?",
        TStringBuilder() << "volumeRemoveClient(\"" << clientId << "\");");
}

void BuildVolumeResetMountSeqNumberButton(
    IOutputStream& out,
    const TString& clientId,
    const ui64 tabletId)
{
    out << "<form method='POST' name='ResetMountSeqNumber_" << clientId << "'>"
        << "<input type='hidden' name='TabletID' value='" << tabletId << "'/>"
        << "<input type='hidden' name='ClientId' value='" << clientId << "'/>"
        << "<input type='hidden' name='action' value='resetmountseqnumber'/>"
        << "<input class='btn btn-primary' type='button' value='Reset' "
        << "data-toggle='modal' data-target='#volume-reset-seqnumber-"
        << clientId << "'/>"
        << "</form>" << Endl;

    BuildConfirmActionDialog(
        out,
        TStringBuilder() << "volume-reset-seqnumber-" << clientId,
        "Reset Mount Seq Number",
        "Are you sure you want to volume mount seq number?",
        TStringBuilder() << "resetMountSeqNumber(\"" << clientId << "\");");
}

void BuildHistoryNextButton(
    IOutputStream& out,
    THistoryLogKey key,
    ui64 tabletId)
{
    auto ts = key.Timestamp.MicroSeconds();
    if (ts) {
        --ts;
    }
    out << "<form method='GET' name='NextHistoryPage' style='display:inline-block'>"
        << "<input type='hidden' name='timestamp' value='" << ts << "'/>"
        << "<input type='hidden' name='next' value='true'/>"
        << "<input type='hidden' name='TabletID' value='" << tabletId << "'/>"
        << "<input class='btn btn-primary display:inline-block' type='submit' value='Next>>'/>"
        << "</form>" << Endl;
}

void BuildStartPartitionsButton(
    IOutputStream& out,
    const TString& diskId,
    const ui64 tabletId)
{
    out << "<form method='POST' name='StartPartitions'>"
        << "<input type='hidden' name='TabletID' value='" << tabletId << "'/>"
        << "<input type='hidden' name='Volume' value='" << diskId << "'/>"
        << "<input type='hidden' name='action' value='startpartitions'/>"
        << "<input class='btn btn-primary' type='button' value='Start Partitions' "
        << "data-toggle='modal' data-target='#start-partitions'/>"
        << "</form>" << Endl;

    BuildConfirmActionDialog(
        out,
        "start-partitions",
        "Start partitions",
        "Are you sure you want to start volume partitions?",
        TStringBuilder() << "startPartitions();");
}

void OutputClientInfo(
    IOutputStream& out,
    const TString& clientId,
    const TString& diskId,
    ui64 clientNo,
    ui64 tabletId,
    NProto::EVolumeMountMode mountMode,
    NProto::EVolumeAccessMode accessMode,
    ui64 disconnectTimestamp,
    bool isStale,
    TVolumeClientState::TPipes pipes)
{
    HTML(out) {
        TABLER() {
            TABLED() { out << clientId; }
            TABLED() {
                out << accessMode;
            }
            TABLED() {
                if (mountMode == NProto::VOLUME_MOUNT_LOCAL) {
                    out << "Local";
                } else {
                    out << "Remote";
                }
            }
            TABLED() {
                auto time = disconnectTimestamp;
                if (time) {
                    out << TInstant::MicroSeconds(time).ToStringLocalUpToSeconds();
                }
            }
            TABLED() {
                if (isStale) {
                    out << "Stale";
                } else {
                    out << "Actual";
                }
            }
            TABLED() {
                UL() {
                    for (const auto p: pipes) {
                        if (p.second.State != TVolumeClientState::EPipeState::DEACTIVATED) {
                            if (p.second.State == TVolumeClientState::EPipeState::WAIT_START) {
                                LI() {
                                    out << p.first << "[Wait]";
                                }
                            } else if (p.second.State == TVolumeClientState::EPipeState::ACTIVE) {
                                LI() {
                                    out << p.first << "[Active]";
                                }
                            }
                        }
                    }
                }
            }
            TABLED() {
                BuildVolumeRemoveClientButton(
                    out,
                    clientNo,
                    diskId,
                    clientId,
                    tabletId);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

void RenderNonreplPartitionInfo(
    const NMon::TEvRemoteHttpInfo::TPtr& ev,
    const TActorContext& ctx,
    const NProto::TVolumeMeta& meta,
    const TNonreplicatedPartitionConfig& config)
{
    TStringStream out;

    HTML(out) {
        BODY() {
            TABLE_CLASS("table table-bordered") {
                TABLEHEAD() {
                    TABLER() {
                        TABLEH() { out << "Property"; }
                        TABLEH() { out << "Value"; }
                    }
                }
                TABLER() {
                    TABLED() { out << "BlockSize"; }
                    TABLED() { out << config.GetBlockSize(); }
                }
                TABLER() {
                    TABLED() { out << "Blocks"; }
                    TABLED() { out << config.GetBlockCount(); }
                }
                TABLER() {
                    TABLED() { out << "IOMode"; }
                    TABLED() { out << meta.GetIOMode(); }
                }
                TABLER() {
                    TABLED() { out << "IOModeTs"; }
                    TABLED() { out << meta.GetIOModeTs(); }
                }
                TABLER() {
                    TABLED() { out << "MuteIOErrors"; }
                    TABLED() { out << meta.GetMuteIOErrors(); }
                }

                auto findMigration =
                    [&] (const TString& deviceId) -> const NProto::TDeviceConfig* {
                        for (const auto& migration: meta.GetMigrations()) {
                            if (migration.GetSourceDeviceId() == deviceId) {
                                return &migration.GetTargetDevice();
                            }
                        }

                        return nullptr;
                    };

                auto outputDevices = [&] (const TDevices& devices) {
                    TABLED() {
                        TABLE_CLASS("table table-bordered") {
                            TABLEHEAD() {
                                TABLER() {
                                    TABLEH() { out << "DeviceNo"; }
                                    TABLEH() { out << "NodeId"; }
                                    TABLEH() { out << "TransportId"; }
                                    TABLEH() { out << "RdmaEndpoint"; }
                                    TABLEH() { out << "DeviceName"; }
                                    TABLEH() { out << "DeviceUUID"; }
                                    TABLEH() { out << "BlockSize"; }
                                    TABLEH() { out << "Blocks"; }
                                }
                            }

                            auto outputDevice = [&] (const NProto::TDeviceConfig& d) {
                                TABLED() { out << d.GetNodeId(); }
                                TABLED() { out << d.GetTransportId(); }
                                TABLED() {
                                    auto e = d.GetRdmaEndpoint();
                                    out << e.GetHost() << ":" << e.GetPort();
                                }
                                TABLED() { out << d.GetDeviceName(); }
                                TABLED() {
                                    const bool isFresh = Find(
                                        meta.GetFreshDeviceIds().begin(),
                                        meta.GetFreshDeviceIds().end(),
                                        d.GetDeviceUUID()
                                    ) != meta.GetFreshDeviceIds().end();

                                    if (isFresh) {
                                        out << "<font color=blue>";
                                    }
                                    out << d.GetDeviceUUID();
                                    if (isFresh) {
                                        out << "</font>";
                                    }
                                }
                                TABLED() { out << d.GetBlockSize(); }
                                TABLED() { out << d.GetBlocksCount(); }
                            };

                            for (int i = 0; i < devices.size(); ++i) {
                                const auto& device = devices[i];

                                TABLER() {
                                    TABLED() { out << i; }
                                    outputDevice(device);
                                }

                                auto* m = findMigration(device.GetDeviceUUID());
                                if (m) {
                                    TABLER() {
                                        TABLED() { out << i << " migration"; }
                                        outputDevice(*m);
                                    }
                                }
                            }
                        }
                    }
                };

                TABLER() {
                    TABLED() { out << "Devices"; }
                    outputDevices(config.GetDevices());
                }

                for (ui32 i = 0; i < meta.ReplicasSize(); ++i) {
                    TABLER() {
                        TABLED() {
                            out << "Devices (Replica " << (i + 1) << ")";
                        }
                        outputDevices(meta.GetReplicas(i).GetDevices());
                    }
                }
            }
        }
    }

    NCloud::Reply(
        ctx,
        *ev,
        std::make_unique<NMon::TEvRemoteHttpInfoRes>(out.Str()));
}

} // namespace

////////////////////////////////////////////////////////////////////////////////

void TVolumeActor::HandleHttpInfo(
    const NMon::TEvRemoteHttpInfo::TPtr& ev,
    const TActorContext& ctx)
{
    using namespace NMonitoringUtils;

    auto* msg = ev->Get();

    LOG_DEBUG(ctx, TBlockStoreComponents::VOLUME,
        "[%lu] HTTP request: %s",
        TabletID(),
        msg->Query.Quote().data());

    TStringStream out;

    if (State) {
        const auto& params = msg->Cgi();

        const auto& clientId = params.Get("ClientId");
        const auto& action = params.Get("action");
        const auto& timestamp = params.Get("timestamp");
        const auto& activeTab = params.Get("tab");
        const auto& checkpointId = params.Get("checkpointid");

        if (action == "rendernpinfo") {
            RenderNonreplPartitionInfo(
                ev,
                ctx,
                State->GetMeta(),
                *State->GetNonreplicatedPartitionConfig()
            );

            return;
        }

        if (clientId && (action == "removeclient")) {
            HandleHttpInfo_RemoveClient(ev, clientId, ctx);
            return;
        }

        if (clientId && (action == "resetmountseqnumber")) {
            HandleHttpInfo_ResetMountSeqNumber(ev, clientId, ctx);
            return;
        }

        if (checkpointId) {
            if (action == "createCheckpoint") {
                HandleHttpInfo_CreateCheckpoint(ev, checkpointId, ctx);
                return;
            } else if (action == "deleteCheckpoint") {
                HandleHttpInfo_DeleteCheckpoint(ev, checkpointId, ctx);
                return;
            }
        }

        if (timestamp) {
            ui64 ts = 0;
            if (TryFromString(timestamp, ts)) {

                auto cancelRoutine =
                [] (const TActorContext& ctx, TRequestInfo& requestInfo)
                {
                    TStringStream out;

                    DumpTabletNotReady(out);
                    NCloud::Reply(
                        ctx,
                        requestInfo,
                        std::make_unique<NMon::TEvRemoteHttpInfoRes>(
                            out.Str()));
                };

                auto requestInfo = CreateRequestInfo(
                    ev->Sender,
                    ev->Cookie,
                    MakeIntrusive<TCallContext>(),
                    std::move(ev->TraceId),
                    cancelRoutine);

                ProcessReadHistory(
                    ctx,
                    std::move(requestInfo),
                    TInstant::MicroSeconds(ts),
                    Config->GetVolumeHistoryCacheSize(),
                    true);
                return;
            }
        }

        if (action == "startpartitions") {
            HandleHttpInfo_StartPartitions(ev, ctx);
            return;
        }

        HandleHttpInfo_Default(
            ctx,
            State->GetHistory(),
            activeTab,
            out);
    } else {
        DumpTabletNotReady(out);
    }

    NCloud::Reply(
        ctx,
        *ev,
        std::make_unique<NMon::TEvRemoteHttpInfoRes>(out.Str()));
}

void TVolumeActor::HandleHttpInfo_Default(
    const NActors::TActorContext& ctx,
    const TDeque<THistoryLogItem>& history,
    const TStringBuf tabName,
    IOutputStream& out)
{
    Y_UNUSED(ctx);

    const char* overviewTabName = "Overview";
    const char* historyTabName = "History";
    const char* checkpointsTabName = "Checkpoints";
    const char* tracesTabName = "Traces";

    const char* activeTab = "tab-pane active";
    const char* inactiveTab = "tab-pane";

    const char* overviewTab = inactiveTab;
    const char* historyTab = inactiveTab;
    const char* checkpointsTab = inactiveTab;
    const char* tracesTab = inactiveTab;

    if (tabName.Empty() || tabName == overviewTabName) {
        overviewTab = activeTab;
    } else if (tabName == historyTabName) {
        historyTab = activeTab;
    } else if (tabName == checkpointsTabName) {
        checkpointsTab = activeTab;
    } else if (tabName == tracesTabName) {
        tracesTab = activeTab;
    }

    HTML(out) {
        DIV_CLASS_ID("container-fluid", "tabs") {
            BuildVolumeTabs(out);

            DIV_CLASS("tab-content") {
                DIV_CLASS_ID(overviewTab, overviewTabName) {
                    DumpDefaultHeader(out, *Info(), SelfId().NodeId(), *DiagnosticsConfig);
                    RenderHtmlInfo(out);
                }

                DIV_CLASS_ID(historyTab, historyTabName) {
                    RenderHistory(history, out);
                }

                DIV_CLASS_ID(checkpointsTab, checkpointsTabName) {
                    RenderCheckpoints(out);
                }

                DIV_CLASS_ID(tracesTab, tracesTabName) {
                    RenderTraces(out);
                }
            }
        }
    }
}

void TVolumeActor::RenderHistory(
    const TDeque<THistoryLogItem>& history,
    IOutputStream& out) const
{
    using namespace NMonitoringUtils;

    HTML(out) {
        DIV_CLASS("row") {
            H3() { out << "History"; }
            TABLE_SORTABLE_CLASS("table table-bordered") {
                TABLEHEAD() {
                    TABLER() {
                        TABLEH() { out << "Time"; }
                        TABLEH() { out << "Operation type"; }
                        TABLEH() { out << "Result"; }
                        TABLEH() { out << "Operation"; }
                    }
                }
                for (const auto& h: history) {
                    TABLER() {
                        TABLED() {
                            out << h.Key.Timestamp;
                        }
                        TABLED() {
                            TStringBuf type;
                            if (h.Operation.HasAdd()) {
                                type = "Add";
                            } else if (h.Operation.HasRemove()) {
                                type = "Remove";
                            } else {
                                type = "Update";
                            }
                            out << type;
                        }
                        TABLED() {
                            if (FAILED(h.Operation.GetError().GetCode())) {
                                SerializeToTextFormat(h.Operation.GetError(), out);
                            } else {
                                out << "OK";
                            }
                        }
                        TABLED() {
                            if (h.Operation.HasAdd()) {
                                SerializeToTextFormat(h.Operation.GetAdd(), out);
                            } else if (h.Operation.HasRemove()) {
                                SerializeToTextFormat(h.Operation.GetRemove(), out);
                            } else {
                                SerializeToTextFormat(h.Operation.GetUpdateVolumeMeta(), out);
                            }
                        }
                    }
                }
            }
            if (history.size()) {
                TABLE_CLASS("table") {
                    TABLER() {
                        TABLED() {
                            BuildHistoryNextButton(out, history.back().Key, TabletID());
                        }
                    }
                }
            }
        }
    }
}

void TVolumeActor::RenderCheckpoints(IOutputStream& out) const
{
    using namespace NMonitoringUtils;

    HTML(out) {
        DIV_CLASS("row") {
            H3() {
                BuildMenuButton(out, "checkpoint-add");
                out << "Checkpoints";
            }
            out << "<div class='collapse form-group' id='checkpoint-add'>";
            BuildCreateCheckpointButton(out, TabletID());
            out << "</div>";

            TABLE_SORTABLE_CLASS("table table-bordered") {
                TABLEHEAD() {
                    TABLER() {
                        TABLEH() { out << "CheckpointId"; }
                        TABLEH() { out << "DataDeleted"; }
                        TABLEH() { out << "Delete"; }
                    }
                }
                for (const auto& [r, dataDeleted]: State->GetActiveCheckpoints().GetAll()) {
                    TABLER() {
                        TABLED() { out << r; }
                        TABLED() { out << (dataDeleted ? "true" : ""); }
                        TABLED() {
                            BuildDeleteCheckpointButton(
                                out,
                                TabletID(),
                                r);
                        }
                    }
                }
            }
        };
        DIV_CLASS("row") {
            H3() { out << "CheckpointRequests"; }
            TABLE_SORTABLE_CLASS("table table-bordered") {
                TABLEHEAD() {
                    TABLER() {
                        TABLEH() { out << "RequestId"; }
                        TABLEH() { out << "CheckpointId"; }
                        TABLEH() { out << "Timestamp"; }
                        TABLEH() { out << "State"; }
                        TABLEH() { out << "Type"; }
                    }
                }
                for (const auto& r: State->GetCheckpointRequests()) {
                    TABLER() {
                        TABLED() { out << r.RequestId; }
                        TABLED() { out << r.CheckpointId; }
                        TABLED() { out << r.Timestamp; }
                        TABLED() { out << static_cast<ui32>(r.State); }
                        TABLED() { out << static_cast<ui32>(r.ReqType); }
                    }
                }
            }
        }
        out << R"___(
            <script type='text/javascript'>
            function createCheckpoint() {
                document.forms['CreateCheckpoint'].submit();
            }
            function deleteCheckpoint(checkpointId) {
                document.forms['DeleteCheckpoint_' + checkpointId].submit();
            }
            </script>
        )___";
    }
}

void TVolumeActor::RenderTraces(IOutputStream& out) const {
    using namespace NMonitoringUtils;
    const auto& diskId = State->GetMeta().GetVolumeConfig().GetDiskId();
    HTML(out) {
        DIV_CLASS("row") {
            H3() {
                out << "<a href=\"../tracelogs/slow?diskId=" << diskId << "\">"
                        "Slow logs for " << diskId << "</a><br>";
                out << "<a href=\"../tracelogs/random?diskId=" << diskId << "\">"
                       "Random samples for " << diskId << "</a>";
            }
        }
    }
}

void TVolumeActor::RenderHtmlInfo(IOutputStream& out) const
{
    using namespace NMonitoringUtils;

    if (!State) {
        return;
    }

    HTML(out) {
        DIV_CLASS("row") {
            DIV_CLASS("col-md-6") {
                DumpSolomonVolumeLink(out, *DiagnosticsConfig, State->GetDiskId());
            }
        }

        DIV_CLASS("row") {
            DIV_CLASS("col-md-6") {
                RenderStatus(out);
            }
        }

        DIV_CLASS("row") {
            DIV_CLASS("col-md-6") {
                RenderMigrationStatus(out);
            }
        }

        DIV_CLASS("row") {
            DIV_CLASS("col-md-6") {
                RenderCommonButtons(out);
            }
        }

        DIV_CLASS("row") {
            DIV_CLASS("col-md-6") {
                RenderConfig(out);
            }
        }

        DIV_CLASS("row") {
            RenderTabletList(out);
        }

        DIV_CLASS("row") {
            RenderClientList(out);
        }

        DIV_CLASS("row") {
            RenderMountSeqNumber(out);
        }
    }
}

void TVolumeActor::RenderTabletList(IOutputStream& out) const
{
    HTML(out) {
        H3() { out << "Partitions"; }
        TABLE_SORTABLE_CLASS("table table-bordered") {
            TABLEHEAD() {
                TABLER() {
                    TABLEH() { out << "Partition"; }
                    TABLEH() { out << "Status"; }
                    TABLEH() { out << "Blocks Count"; }
                }
            }

            for (const auto& partition: State->GetPartitions()) {
                TABLER() {
                    TABLED() {
                        out << "<a href='../tablets?TabletID="
                            << partition.TabletId
                            << "'>"
                            << partition.TabletId
                            << "</a>";
                    }
                    TABLED() {
                        out << partition.GetStatus();
                    }
                    TABLED() {
                        out << partition.PartitionConfig.GetBlocksCount();
                    }
                }
            }

            const auto mediaKind = State->GetConfig().GetStorageMediaKind();
            if (IsDiskRegistryMediaKind(mediaKind)) {
                TABLER() {
                    TABLED() {
                        out << "<a href='?action=rendernpinfo"
                            << "&TabletID=" << TabletID()
                            << "'>"
                            << "nonreplicated partition"
                            << "</a>";
                    }
                    TABLED() {
                    }
                    TABLED() {
                        out << State->GetConfig().GetBlocksCount();
                    }
                }
            }
        }
    }
}

void TVolumeActor::RenderConfig(IOutputStream& out) const
{
    if (!State) {
        return;
    }

    const auto& volumeConfig = State->GetMeta().GetVolumeConfig();

    ui64 blocksCount = GetBlocksCount();
    ui32 blockSize = volumeConfig.GetBlockSize();
    const double blocksPerStripe = volumeConfig.GetBlocksPerStripe();
    const double stripes = blocksPerStripe
        ? double(blocksCount) / blocksPerStripe
        : 0;

    HTML(out) {
        H3() { out << "Volume Config"; }
        TABLE_SORTABLE_CLASS("table table-condensed") {
            TABLEHEAD() {
                TABLER() {
                    TABLED() { out << "Disk Id"; }
                    TABLED() { out << volumeConfig.GetDiskId(); }
                }
                TABLER() {
                    TABLED() { out << "Base Disk Id"; }
                    TABLED() { out << volumeConfig.GetBaseDiskId(); }
                }
                TABLER() {
                    TABLED() { out << "Base Disk Checkpoint Id"; }
                    TABLED() { out << volumeConfig.GetBaseDiskCheckpointId(); }
                }
                TABLER() {
                    TABLED() { out << "Folder Id"; }
                    TABLED() { out << volumeConfig.GetFolderId(); }
                }
                TABLER() {
                    TABLED() { out << "Cloud Id"; }
                    TABLED() { out << volumeConfig.GetCloudId(); }
                }
                TABLER() {
                    TABLED() { out << "Project Id"; }
                    TABLED() { out << volumeConfig.GetProjectId(); }
                }

                TABLER() {
                    TABLED() { out << "Block size"; }
                    TABLED() { out << blockSize; }
                }

                TABLER() {
                    TABLED() { out << "Blocks count"; }
                    TABLED() {
                        out << blocksCount;
                        out << " ("
                            << FormatByteSize(blocksCount * blockSize)
                            << ")";
                    }
                }

                if (State->GetUsedBlocks()) {
                    const auto used = State->GetUsedBlocks()->Count();

                    TABLER() {
                        TABLED() { out << "Used blocks count"; }
                        TABLED() {
                            out << used;
                            out << " ("
                                << FormatByteSize(used * blockSize)
                                << ")";
                        }
                    }
                }

                TABLER() {
                    TABLED() { out << "BlocksPerStripe"; }
                    TABLED() {
                        out << blocksPerStripe;
                        out << " ("
                            << FormatByteSize(blocksPerStripe * blockSize)
                            << ")";
                    }
                }

                TABLER() {
                    TABLED() { out << "Stripes"; }
                    TABLED() { out << stripes << Endl; }
                }

                TABLER() {
                    TABLED() { out << "StripesPerPartition"; }
                    TABLED() {
                        out << (stripes / volumeConfig.PartitionsSize()) << Endl;
                    }
                }

                TABLER() {
                    TABLED() { out << "Number of channels"; }
                    TABLED() { out << volumeConfig.ExplicitChannelProfilesSize(); }
                }

                TABLER() {
                    TABLED() { out << "Storage media kind"; }
                    TABLED() {
                        out << NCloud::NProto::EStorageMediaKind_Name(
                            (NCloud::NProto::EStorageMediaKind)volumeConfig.GetStorageMediaKind());
                    }
                }

                TABLER() {
                    TABLED() { out << "Channel profile id"; }
                    TABLED() {
                        out << volumeConfig.GetChannelProfileId();
                    }
                }

                TABLER() {
                    TABLED() { out << "Partition tablet version"; }
                    TABLED() {
                        out << volumeConfig.GetTabletVersion();
                    }
                }

                TABLER() {
                    TABLED() { out << "Tags"; }
                    TABLED() {
                        out << volumeConfig.GetTagsStr();
                    }
                }

                TABLER() {
                    TABLED() { out << "UseRdma"; }
                    TABLED() {
                        out << State->GetUseRdma();
                    }
                }

                TABLER() {
                    TABLED() { out << "Throttler"; }
                    TABLED() {
                        const auto& tp = State->GetThrottlingPolicy();
                        TABLE_CLASS("table table-condensed") {
                            TABLEBODY() {
                                TABLER() {
                                    TABLED() { out << "PostponedQueueSize"; }
                                    TABLED() { out << PostponedRequests.size(); }
                                }
                                TABLER() {
                                    TABLED() { out << "WriteCostMultiplier"; }
                                    TABLED() { out << tp.GetWriteCostMultiplier(); }
                                }
                                TABLER() {
                                    TABLED() { out << "PostponedQueueWeight"; }
                                    TABLED() { out << tp.CalculatePostponedWeight(); }
                                }
                                TABLER() {
                                    TABLED() { out << "BackpressureFeatures"; }
                                    TABLED() {
                                        const auto& bp = tp.GetCurrentBackpressure();
                                        TABLE_CLASS("table table-condensed") {
                                            TABLEBODY() {
                                                TABLER() {
                                                    TABLED() { out << "FreshIndexScore"; }
                                                    TABLED() { out << bp.FreshIndexScore; }
                                                }
                                                TABLER() {
                                                    TABLED() { out << "CompactionScore"; }
                                                    TABLED() { out << bp.CompactionScore; }
                                                }
                                                TABLER() {
                                                    TABLED() { out << "DiskSpaceScore"; }
                                                    TABLED() { out << bp.DiskSpaceScore; }
                                                }
                                            }
                                        }
                                    }
                                }
                                TABLER() {
                                    TABLED() { out << "ThrottlerParams"; }
                                    TABLED() {
                                        using EOpType = TVolumeThrottlingPolicy::EOpType;

                                        TABLE_CLASS("table table-condensed") {
                                            TABLEBODY() {
                                                TABLER() {
                                                    TABLED() { out << "ReadC1"; }
                                                    TABLED() { out << tp.C1(EOpType::Read); }
                                                }
                                                TABLER() {
                                                    TABLED() { out << "ReadC2"; }
                                                    TABLED() { out << FormatByteSize(tp.C2(EOpType::Read)); }
                                                }
                                                TABLER() {
                                                    TABLED() { out << "WriteC1"; }
                                                    TABLED() { out << tp.C1(EOpType::Write); }
                                                }
                                                TABLER() {
                                                    TABLED() { out << "WriteC2"; }
                                                    TABLED() { out << FormatByteSize(tp.C2(EOpType::Write)); }
                                                }
                                                TABLER() {
                                                    TABLED() { out << "DescribeC1"; }
                                                    TABLED() { out << tp.C1(EOpType::Describe); }
                                                }
                                                TABLER() {
                                                    TABLED() { out << "DescribeC2"; }
                                                    TABLED() { out << FormatByteSize(tp.C2(EOpType::Describe)); }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                TABLER() {
                    TABLED() { out << "ExplicitChannelProfiles"; }
                    TABLED() {
                        if (volumeConfig.ExplicitChannelProfilesSize()) {
                            out << "Yes";
                        } else {
                            out << "No";
                        }
                    }
                }
            }
        }
    }
}

void TVolumeActor::RenderClientList(IOutputStream& out) const
{
    if (!State) {
        return;
    }

    const auto& clients = State->GetClients();
    if (clients.empty()) {
        return;
    }

    const auto& diskId = State->GetDiskId();

    HTML(out) {
        H3() { out << "Volume Clients"; }
        TABLE_SORTABLE_CLASS("table table-condensed") {
            TABLEHEAD() {
                TABLER() {
                    TABLEH() { out << "Client Id"; }
                    TABLEH() { out << "Access mode"; }
                    TABLEH() { out << "Mount mode"; }
                    TABLEH() { out << "Disconnect time"; }
                    TABLEH() { out << "State"; }
                    TABLEH() { out << "Pipe server actor"; }
                    TABLEH() { out << "Remove"; }
                }
            }
            ui64 clientNo = 0;
            for (const auto& pair: clients) {
                const auto& clientId = pair.first;
                const auto& clientInfo = pair.second;
                const auto& volumeClientInfo = clientInfo.GetVolumeClientInfo();

                OutputClientInfo(
                    out,
                    clientId,
                    diskId,
                    clientNo++,
                    TabletID(),
                    volumeClientInfo.GetVolumeMountMode(),
                    volumeClientInfo.GetVolumeAccessMode(),
                    volumeClientInfo.GetDisconnectTimestamp(),
                    State->IsClientStale(clientId),
                    clientInfo.GetPipes());
            }
        }

        out << R"___(
                <script>
                function volumeRemoveClient(clientId) {
                    document.forms["RemoveClient_" + clientId].submit();
                }
                </script>
            )___";
    }
}

void TVolumeActor::RenderMountSeqNumber(IOutputStream& out) const
{
    if (!State) {
        return;
    }

    const auto& clients = State->GetClients();
    if (clients.empty()) {
        return;
    }

    HTML(out) {
        H3() { out << "Volume Mount Sequence Number"; }
        TABLE_SORTABLE_CLASS("table table-condensed") {
            TABLEHEAD() {
                TABLER() {
                    TABLEH() { out << "Client Id"; }
                    TABLEH() { out << "Mount Generation"; }
                    TABLEH() { out << "Reset"; }
                }
            }

            TABLER() {
                const auto& rwClient = State->GetReadWriteAccessClientId();
                TABLED() { out << rwClient; }
                TABLED() { out << State->GetMountSeqNumber(); }
                TABLED() {
                    BuildVolumeResetMountSeqNumberButton(
                        out,
                        rwClient,
                        TabletID());
                }
            }
        }

        out << R"___(
                <script>
                function resetMountSeqNumber(clientId) {
                    document.forms["ResetMountSeqNumber_" + clientId].submit();
                }
                </script>
            )___";
    }
}

void TVolumeActor::RenderStatus(IOutputStream& out) const
{
    HTML(out) {
        H3() {
            TString statusText = "offline";
            TString cssClass = "label-danger";

            auto status = GetVolumeStatus();
            if (status == TVolumeActor::STATUS_INACTIVE) {
                statusText = GetVolumeStatusString(status);
                cssClass = "label-default";
            } else if (status != TVolumeActor::STATUS_OFFLINE) {
                statusText = GetVolumeStatusString(status);
                cssClass = "label-success";
            }

            out << "Status:";

            SPAN_CLASS_STYLE("label " + cssClass, "margin-left:10px") {
                out << statusText;
            }
        }
    }
}

void TVolumeActor::RenderMigrationStatus(IOutputStream& out) const
{
    HTML(out) {
        H3() {
            TString statusText = "inactive";
            TString cssClass = "label-default";

            if (State->GetMeta().GetMigrations().size()) {
                statusText = "in progress";
                cssClass = "label-success";
            }

            out << "MigrationStatus:";

            SPAN_CLASS_STYLE("label " + cssClass, "margin-left:10px") {
                out << statusText;
            }
        }

        if (State->GetMeta().GetMigrations().empty()) {
            return;
        }

        const auto totalBlocks = GetBlocksCount();
        const auto migrationIndex = State->GetMeta().GetMigrationIndex();
        const auto blockSize = State->GetBlockSize();

        auto outputProgress = [&] (ui64 progress) {
            out << progress;
            out << " ("
                << FormatByteSize(progress * blockSize)
                << ")";
            out << " / ";
            out << totalBlocks;
            out << " ("
                << FormatByteSize(totalBlocks * blockSize)
                << ")";
            out << " = ";
            out << (100 * progress / totalBlocks) << "%";
        };

        TABLE_SORTABLE_CLASS("table table-condensed") {
            TABLEHEAD() {
                TABLER() {
                    TABLED() { out << "Migration index: "; }
                    TABLED() { outputProgress(migrationIndex); }
                }
            }
        }
    }
}

void TVolumeActor::RenderCommonButtons(IOutputStream& out) const
{
    if (!State) {
        return;
    }

    HTML(out) {
        H3() { out << "Controls"; }
        TABLE_SORTABLE_CLASS("table table-condensed") {
            TABLER() {
                TABLED() {
                    BuildStartPartitionsButton(
                        out,
                        State->GetDiskId(),
                        TabletID());
                }
            }
        }

        out << R"___(
                <script>
                function startPartitions() {
                    document.forms["StartPartitions"].submit();
                }
                </script>
            )___";
    }
}

void TVolumeActor::RejectHttpRequest(
    const TActorId& sender,
    const TActorContext& ctx)
{
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
    NCloud::Send(ctx, sender, std::move(response));
}

void TVolumeActor::HandleHttpInfo_StartPartitions(
    const NMon::TEvRemoteHttpInfo::TPtr& ev,
    const TActorContext& ctx)
{
    StartPartitionsIfNeeded(ctx);

    TStringStream out;
    BuildTabletNotifyPageWithRedirect(
        out,
        "Start initiated",
        TabletID(),
        EAlertLevel::SUCCESS);

    NCloud::Reply(
        ctx,
        *ev,
        std::make_unique<NMon::TEvRemoteHttpInfoRes>(out.Str()));
}

}   // namespace NCloud::NBlockStore::NStorage
