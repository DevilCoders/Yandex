#include "event2yt.h"

#include <mapreduce/yt/common/helpers.h>
#include <mapreduce/yt/interface/node.h>

#include <library/cpp/eventlog/eventlog.h>
#include <library/cpp/protobuf/yt/proto2yt.h>

#include <util/generic/string.h>
#include <util/stream/str.h>

static NYT::TNode ProtoEventToYtNode(const TEvent& event) {
    NYT::TNode ret;
    ret["Timestamp"] = event.Timestamp;
    ret["FrameId"] = event.FrameId;

    auto& body = ret["EventBody"];
    body["Type"] = event.GetName();
    body["Fields"] = ProtoToYtNode(*event.GetProto());

    return ret;
}

static NYT::TNode JsonEventToYtNode(const TEvent& event) {
    TString buffer;
    {
        TStringOutput output(buffer);
        event.Print(output, TEvent::TOutputFormat::Json);
    }
    return NYT::NodeFromJsonString(buffer);
}

NYT::TNode EventToYtNode(const TEvent& event) {
    auto ret = event.GetProto() ? ProtoEventToYtNode(event) : JsonEventToYtNode(event);
    return ret;
}
