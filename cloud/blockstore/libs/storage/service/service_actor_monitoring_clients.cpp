#include "service_actor.h"

#include <cloud/blockstore/libs/common/proto_helpers.h>
#include <cloud/blockstore/libs/service/request_helpers.h>
#include <cloud/blockstore/libs/storage/core/monitoring_utils.h>

#include <util/generic/vector.h>
#include <util/stream/str.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;
using namespace NMonitoringUtils;

namespace {

////////////////////////////////////////////////////////////////////////////////

void BuildUnmountVolumeButton(
    IOutputStream& out,
    ui64 id,
    const TString& diskId,
    const TString& clientId)
{
    out << "<form method='POST' name='UnmountVolume_" << clientId << "'>"
        << "<input type='hidden' name='ClientId' value='" << clientId << "'/>"
        << "<input type='hidden' name='Volume' value='" << diskId << "'/>"
        << "<input type='hidden' name='action' value='unmount'/>"
        << "<input class='btn btn-primary' type='button' value='Unmount' "
        << "data-toggle='modal' data-target='#unmount-client-" << id << "'/>"
        << "</form>" << Endl;

    BuildConfirmActionDialog(
        out,
        TStringBuilder() << "unmount-client-" << id,
        "Unmount volume",
        "Are you sure you want to unmount volume?",
        TStringBuilder() << "unmountVolume(\"" << clientId << "\");");
}

TString GetIpcTypeName(NProto::EClientIpcType ipcType)
{
    switch (ipcType)
    {
        case NProto::IPC_GRPC:
            return "GRPC";

        case NProto::IPC_VHOST:
            return "VHOST";

        case NProto::IPC_NBD:
            return "NBD";

        case NProto::IPC_NVME:
            return "NVME";

        case NProto::IPC_SCSI:
            return "SCSI";

        case NProto::IPC_RDMA:
            return "RDMA";

        default:
            return "Undefined";
    }
}

void BuildHtmlResponse(
    const TString& diskId,
    const TClientInfoList& clientInfos,
    TStringStream& out)
{
    HTML(out) {
        H3() { out << "Volume " << diskId << " clients"; }
        TABLE_SORTABLE_CLASS("table table-bordered") {
            TABLEHEAD() {
                TABLER() {
                    TABLEH() { out << "Client ID"; }
                    TABLEH() { out << "Access mode"; }
                    TABLEH() { out << "Mount mode"; }
                    TABLEH() { out << "Mount options"; }
                    TABLEH() { out << "Last activity time"; }
                    TABLEH() { out << "IPC type"; }
                    TABLEH() { out << "Version"; }
                    TABLEH() { out << "Unmount"; }
                }
            }
            ui64 clientNo = 0;
            for (const auto& clientInfo: clientInfos) {
                bool readOnly = !IsReadWriteMode(clientInfo.VolumeAccessMode);
                bool mountLocal =
                    (clientInfo.VolumeMountMode == NProto::VOLUME_MOUNT_LOCAL);
                bool throttlingDisabled = HasProtoFlag(
                    clientInfo.MountFlags,
                    NProto::MF_THROTTLING_DISABLED);
                bool forceWrite = HasProtoFlag(
                    clientInfo.MountFlags,
                    NProto::MF_FORCE_WRITE);
                auto ipcTypeName = GetIpcTypeName(clientInfo.IpcType);
                TABLER() {
                    TABLED() { out << clientInfo.ClientId; }
                    TABLED() { out << (readOnly ? "Read only" : "Read/Write"); }
                    TABLED() { out << (mountLocal ? "Local" : "Remote"); }
                    TABLED() {
                        out << "Throttling"
                            << (throttlingDisabled ? "Disabled" : "Enabled");
                    }
                    TABLED() {
                        out << "ForceWrite" << (forceWrite ? "On" : "Off");
                    }
                    TABLED() {
                        out << (clientInfo.LastActivityTime.ToStringLocalUpToSeconds());
                    }
                    TABLED() { out << ipcTypeName; }
                    TABLED() { out << clientInfo.ClientVersionInfo; }
                    TABLED() {
                        BuildUnmountVolumeButton(
                            out,
                            clientNo++,
                            diskId,
                            clientInfo.ClientId);
                    }
                }
            }
        }

        out << R"___(
                <script>
                function unmountVolume(clientId) {
                    document.forms["UnmountVolume_" + clientId].submit();
                }
                </script>
            )___";
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TServiceActor::HandleHttpInfo_Clients(
    const NMon::TEvHttpInfo::TPtr& ev,
    const TString& diskId,
    const TActorContext& ctx)
{
    const auto volume = State.GetVolume(diskId);
    if (!volume) {
        TStringStream out;
        BuildNotifyPageWithRedirect(
            out,
            TStringBuilder() << "Volume " << diskId << " not found",
            TStringBuilder() << "../blockstore/service",
            EAlertLevel::DANGER);

        auto response = std::make_unique<NMon::TEvHttpInfoRes>(out.Str());
        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    TStringStream out;
    BuildHtmlResponse(diskId, volume->ClientInfos, out);
    auto response = std::make_unique<NMon::TEvHttpInfoRes>(out.Str());
    NCloud::Reply(ctx, *ev, std::move(response));
}

}   // namespace NCloud::NBlockStore::NStorage
