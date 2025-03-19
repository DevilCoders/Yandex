#include "dump.h"

#include <cloud/filestore/libs/diagnostics/events/profile_events.ev.pb.h>
#include <cloud/filestore/libs/service/request.h>

#include <cloud/storage/core/libs/common/error.h>

#include <util/datetime/base.h>
#include <util/generic/algorithm.h>
#include <util/string/printf.h>

namespace NCloud::NFileStore {

namespace {

////////////////////////////////////////////////////////////////////////////////

void OutputNodeInfo(
    const NProto::TProfileLogNodeInfo& nodeInfo,
    IOutputStream* out)
{
    if (nodeInfo.ByteSize()) {
        (*out) << "p=" << nodeInfo.GetParentNodeId()
            << ",n=" << nodeInfo.GetNodeName()
            << ",pp=" << nodeInfo.GetNewParentNodeId()
            << ",nn=" << nodeInfo.GetNewNodeName()
            << ",i=" << nodeInfo.GetNodeId();
    } else {
        (*out) << "no_node_info";
    }
}

using TRanges =
    google::protobuf::RepeatedPtrField<NProto::TProfileLogBlockRange>;

void OutputRanges(const TRanges& ranges, IOutputStream* out)
{
    for (int i = 0; i < ranges.size(); ++i) {
        if (i) {
            (*out) << " ";
        }
        (*out) << ranges[i].GetNodeId()
            << "," << ranges[i].GetOffset()
            << "," << ranges[i].GetBytes();
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

TVector<ui32> GetItemOrder(const NProto::TProfileLogRecord& record)
{
    TVector<ui32> order;
    order.reserve(record.RequestsSize());

    for (ui32 i = 0; i < record.RequestsSize(); ++i) {
        order.emplace_back(i);
    }

    Sort(
        order.begin(),
        order.end(),
        [&] (const auto& i, const auto& j) {
            return record.GetRequests(i).GetTimestampMcs()
                < record.GetRequests(j).GetTimestampMcs();
        }
    );

    return order;
}

void DumpRequest(
    const NProto::TProfileLogRecord& record,
    int i,
    IOutputStream* out)
{
    const auto& r = record.GetRequests(i);

    (*out) << TInstant::MicroSeconds(r.GetTimestampMcs())
        << "\t" << record.GetFileSystemId()
        << "\t" << RequestName(r.GetRequestType())
        << "\t" << r.GetDurationMcs()
        << "\t" << FormatResultCode(r.GetErrorCode())
        << "\t";

    OutputNodeInfo(r.GetNodeInfo(), out);
    (*out) << "\t";

    OutputRanges(r.GetRanges(), out);
    (*out) << "\n";
}

TString RequestName(const ui32 requestType)
{
    TString name;
    if (requestType < static_cast<int>(EFileStoreRequest::MAX)) {
        name = GetFileStoreRequestName(
            static_cast<EFileStoreRequest>(requestType)
        );
    } else if (requestType < static_cast<int>(EFileStoreSystemRequest::MAX)) {
        name = GetFileStoreSystemRequestName(
            static_cast<EFileStoreSystemRequest>(requestType)
        );
    }

    // XXX
    if (name.StartsWith("Unknown")) {
        name = Sprintf("Unknown-%u", requestType);
    }

    return name;
}

}   // namespace NCloud::NFileStore
