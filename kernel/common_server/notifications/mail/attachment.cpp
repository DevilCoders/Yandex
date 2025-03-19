#include "attachment.h"

#include <kernel/common_server/util/json_processing.h>
#include <library/cpp/string_utils/base64/base64.h>

NJson::TJsonValue TFileAttachment::SerializeToJson() const {
    NJson::TJsonValue json;
    JWRITE(json, "filename", FileName);
    JWRITE(json, "mime_type", MimeType);
    JWRITE(json, "data", Base64Encode(Data.AsStringBuf()));
    return json;
}

bool TFileAttachment::DeserializeFromJson(const NJson::TJsonValue& json) {
    JREAD_STRING(json, "filename", FileName);
    JREAD_STRING(json, "mime_type", MimeType);
    if (json.Has("data") && json["data"].IsString()) {
        Data = TBlob::FromString(Base64StrictDecode(json["data"].GetString()));
        return true;
    }
    return false;
}

void TFileAttachment::SerializeToProto(NCommonServerProto::TFileAttachment& proto) const {
    proto.SetData(TString(Data.AsCharPtr(), Data.Size()));
    proto.SetMimeType(MimeType);
    proto.SetFileName(FileName);
}

void TFileAttachment::DeserializeFromProto(const NCommonServerProto::TFileAttachment& proto) {
    Data = TBlob::FromString(proto.GetData());
    MimeType = proto.GetMimeType();
    FileName = proto.GetFileName();
}
