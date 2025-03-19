#include "service_actor.h"

#include <cloud/blockstore/libs/diagnostics/volume_stats.h>
#include <cloud/blockstore/libs/storage/core/monitoring_utils.h>
#include <cloud/blockstore/libs/storage/core/proto_helpers.h>
#include <cloud/storage/core/libs/common/format.h>
#include <cloud/storage/core/libs/common/media.h>

#include <library/cpp/monlib/service/pages/templates.h>

#include <util/stream/str.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;
using namespace NMonitoringUtils;

namespace {

////////////////////////////////////////////////////////////////////////////////

void BuildSearchButton(IOutputStream& out)
{
    out <<
        "<form method=\"GET\" id=\"volSearch\" name=\"volSearch\">\n"
        "Volume: <input type=\"text\" id=\"Volume\" name=\"Volume\"/>\n"
        "<input class=\"btn btn-primary\" type=\"submit\" value=\"Search\"/>\n"
        "<input type='hidden' name='action' value='search'/>"
        "</form>\n";
}

void BuildVolumePreemptionButton(IOutputStream& out, const TVolumeInfo& volume)
{
    TString buttonName = "Push";
    if (volume.BindingType == NProto::BINDING_REMOTE) {
        buttonName = "Pull";
    };

    out << "<form method='POST' name='volPreempt" << volume.DiskId << "'>\n";
    out << "<input class='btn btn-primary' type='button' value='" << buttonName << "'"
        << " data-toggle='modal' data-target='#toggle-preemption" << volume.DiskId << "'/>";
    out << "<input type='hidden' name='action' value='togglePreemption'/>";
    out << "<input type='hidden' name='type' value='" << buttonName << "'/>";
    out << "<input type='hidden' name='Volume' value='" << volume.DiskId << "'/>";
    out << "</form>\n";

    BuildConfirmActionDialog(
        out,
        TStringBuilder() << "toggle-preemption" << volume.DiskId,
        "toggle-preemption",
        TStringBuilder() << "Are you sure you want to " << buttonName << " volume?",
        TStringBuilder() << "togglePreemption(\"" << volume.DiskId << "\");");
}

void GenerateServiceActionsJS(IOutputStream& out)
{
    out << R"___(
        <script type='text/javascript'>
        function togglePreemption(diskId) {
            document.forms['volPreempt'+diskId].submit();
        }
        </script>
    )___";
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TServiceActor::HandleHttpInfo(
    const NMon::TEvHttpInfo::TPtr& ev,
    const TActorContext& ctx)
{
    const auto& request = ev->Get()->Request;
    TString uri{request.GetUri()};
    LOG_DEBUG(ctx, TBlockStoreComponents::SERVICE,
        "HTTP request: %s", uri.c_str());

    const auto& params =
        (request.GetMethod() != HTTP_METHOD_POST) ? request.GetParams() : request.GetPostParams();

    const auto& clientId = params.Get("ClientId");
    const auto& diskId = params.Get("Volume");
    const auto& action = params.Get("action");

    LOG_DEBUG(ctx, TBlockStoreComponents::SERVICE,
        "HTTP request action: %s", action.c_str());
    LOG_DEBUG(ctx, TBlockStoreComponents::SERVICE,
        "HTTP request diskId: %s", diskId.c_str());

    if (clientId && diskId && (action == "unmount")) {
        HandleHttpInfo_Unmount(ev, diskId, clientId, ctx);
        return;
    }

    if (diskId && (action == "listclients")) {
        HandleHttpInfo_Clients(ev, diskId, ctx);
        return;
    }

    if (diskId && (action == "search")) {
        HandleHttpInfo_Search(ev, diskId, ctx);
        return;
    }

    if (diskId && (action == "togglePreemption")) {
        const auto& actionType = params.Get("type");
        HandleHttpInfo_VolumeBinding(ev, diskId, actionType, ctx);
        return;
    }

    TStringStream out;
    RenderHtmlInfo(out);

    NCloud::Reply(
        ctx,
        *ev,
        std::make_unique<NMon::TEvHttpInfoRes>(out.Str()));
}

void TServiceActor::RenderHtmlInfo(IOutputStream& out) const
{
    HTML(out) {
        H3() { out << "Search Volume"; }
        BuildSearchButton(out);

        H3() { out << "Volumes"; }
        RenderVolumeList(out);

        H3() { out << "Config"; }
        Config->DumpHtml(out);

        if (Counters) {
            H3() { out << "Counters"; }
            Counters->OutputHtml(out);
        }
        GenerateServiceActionsJS(out);
    }
}

void TServiceActor::RenderVolumeList(IOutputStream& out) const
{
    auto status = VolumeStats->GatherVolumePerfStatuses();
    THashMap<TString, bool> statusMap(status.begin(), status.end());

    HTML(out) {
        TABLE_SORTABLE_CLASS("table table-bordered") {
            TABLEHEAD() {
                TABLER() {
                    TABLEH() { out << "Volume"; }
                    TABLEH() { out << "Tablet"; }
                    TABLEH() { out << "Size"; }
                    TABLEH() { out << "PartitionCount"; }
                    TABLEH() { out << "Media kind"; }
                    TABLEH() { out << "Status"; }
                    TABLEH() { out << "Clients"; }
                    TABLEH() { out << "Meets perf guarantees"; }
                    TABLEH() { out << "Action"; }
                }
            }

            for (const auto& p: State.GetVolumes()) {
                const auto& volume = *p.second;
                if (!volume.VolumeInfo.Defined()) {
                    continue;
                }
                TABLER() {
                    TABLED() {
                        out << "<a href='../tablets?TabletID="
                            << volume.TabletId
                            << "'>"
                            << volume.VolumeInfo->GetDiskId()
                            << "</a>";
                    }
                    TABLED() {
                        out << "<a href='../tablets?TabletID="
                            << volume.TabletId
                            << "'>"
                            << volume.TabletId
                            << "</a>";
                    }
                    TABLED() {
                        out << FormatByteSize(
                            volume.VolumeInfo->GetBlocksCount() * volume.VolumeInfo->GetBlockSize());
                    }
                    TABLED() {
                        out << volume.VolumeInfo->GetPartitionsCount();
                    }
                    TABLED() {
                        out << MediaKindToString(volume.VolumeInfo->GetStorageMediaKind());
                    }
                    TABLED() {
                        TString statusText = "Online";
                        TString cssClass = "label-info";

                        if (volume.GetLocalMountClientInfo()) {
                            if (volume.BindingType != NProto::BINDING_REMOTE) {
                                statusText = "Mounted";
                            } else {
                                switch (volume.PreemptionSource) {
                                    case NProto::SOURCE_INITIAL_MOUNT: {
                                        statusText = "Mounted (initial)";
                                        break;
                                    }
                                    case NProto::SOURCE_BALANCER: {
                                        statusText = "Mounted (preempted)";
                                        break;
                                    }
                                    case NProto::SOURCE_MANUAL: {
                                        statusText = "Mounted (manually preempted)";
                                        break;
                                    }
                                    default: {
                                        statusText = "Mounted (not set)";
                                    }
                                }
                            }
                            cssClass = "label-success";
                        } else if (volume.IsMounted()) {
                            statusText = "Mounted (remote)";
                        }

                        SPAN_CLASS("label " + cssClass) {
                            out << statusText;
                        }
                    }
                    TABLED() {
                        out << "<a href='../blockstore/service?Volume="
                            << volume.VolumeInfo->GetDiskId() << "&action=listclients'>"
                            << volume.ClientInfos.Size()
                            << "</a>";
                    }
                    TABLED() {
                        TString statusText = "Unknown";
                        TString cssClass = "label-default";

                        auto it = statusMap.find(volume.VolumeInfo->GetDiskId());
                        if (it != statusMap.end()) {
                            if (!it->second) {
                                statusText = "Yes";
                                cssClass = "label-success";
                            } else {
                                statusText = "No";
                                cssClass = "label-warning";
                            }
                        }

                        SPAN_CLASS("label " + cssClass) {
                            out << statusText;
                        }
                    }
                    TABLED() {
                        if (volume.GetLocalMountClientInfo()) {
                            BuildVolumePreemptionButton(out, volume);
                        }
                    }
                }
            }
        }
    }
}

void TServiceActor::RejectHttpRequest(
    const TActorId& sender,
    const TActorContext& ctx)
{
    using namespace NMonitoringUtils;

    TString msg = "Wrong HTTP method";
    LOG_ERROR_S(ctx, TBlockStoreComponents::SERVICE, msg);

    TStringStream out;
    BuildNotifyPageWithRedirect(
        out,
        msg,
        "../blockstore/service",
        EAlertLevel::DANGER);

    auto response = std::make_unique<NMon::TEvHttpInfoRes>(out.Str());
    NCloud::Send(ctx, sender, std::move(response));
}

}   // namespace NCloud::NBlockStore::NStorage
