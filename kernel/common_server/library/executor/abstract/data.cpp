#include "task.h"
#include <library/cpp/logger/global/global.h>

TInstant IDistributedData::GetDeadline() const {
    CHECK_WITH_LOG(HasDeadline());
    return Deadline;
}

NJson::TJsonValue IDistributedData::GetDataInfo(const TCgiParameters* cgi /*= nullptr*/) const {
    NJson::TJsonValue result;
    NJson::TJsonValue dataJson = DoGetInfo(cgi);
    NJson::TJsonValue metaJson;
    metaJson.InsertValue("type", Type);
    metaJson.InsertValue("id", Identifier);
    if (GetIsFinished()) {
        metaJson.InsertValue("finished", Finished.Seconds());
        metaJson.InsertValue("finished_hr", Finished.ToString());
    }
    if (HasDeadline()) {
        metaJson.InsertValue("deadline", Deadline.Seconds());
        metaJson.InsertValue("deadline_hr", Deadline.ToString());
    }
    result.InsertValue("meta", std::move(metaJson));
    result.InsertValue("data", std::move(dataJson));
    return result;
}

void IDistributedData::SerializeMetaToProto(NFrontendProto::TDataMeta& proto) const {
    proto.SetType(Type);
    proto.SetIdentifier(Identifier);
    proto.SetFinished(Finished.Seconds());
    proto.SetDeadline(Deadline.Seconds());
}

bool IDistributedData::ParseMetaFromProto(const NFrontendProto::TDataMeta& proto) {
    if (Type != proto.GetType()) {
        return false;
    }
    if (Identifier != proto.GetIdentifier()) {
        return false;
    }
    Finished = TInstant::Seconds(proto.GetFinished());
    Deadline = TInstant::Seconds(proto.GetDeadline());
    return true;
}
