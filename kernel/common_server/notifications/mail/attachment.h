#pragma once

#include <kernel/common_server/proto/attachment.pb.h>
#include <kernel/common_server/util/accessor.h>
#include <library/cpp/json/json_value.h>
#include <util/memory/blob.h>

class TFileAttachment {
private:
    CSA_DEFAULT(TFileAttachment, TBlob, Data);
    CSA_DEFAULT(TFileAttachment, TString, MimeType);
    CSA_DEFAULT(TFileAttachment, TString, FileName);
public:
    NJson::TJsonValue SerializeToJson() const;
    Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& json);
    void SerializeToProto(NCommonServerProto::TFileAttachment& proto) const;
    void DeserializeFromProto(const NCommonServerProto::TFileAttachment& proto) ;
};
