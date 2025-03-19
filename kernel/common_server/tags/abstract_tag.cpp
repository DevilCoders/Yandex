#include "abstract_tag.h"
#include "manager.h"

bool ITag::DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values) {
    READ_DECODER_VALUE(decoder, values, Name);
    READ_DECODER_VALUE(decoder, values, Comments);
    if (!Name) {
        TFLEventLog::Error("empty tag name");
        return false;
    }
    return true;
}

NCS::NStorage::TTableRecord ITag::SerializeToTableRecord() const {
    NCS::NStorage::TTableRecord result;
    result.Set("tag_name", Name);
    result.Set("comments", Comments);
    return result;
}

NFrontend::TScheme ITag::GetScheme(const IBaseServer& server) const {
    NFrontend::TScheme scheme;
    scheme.Add<TFSVariants>("tag_name").SetVariants(server.GetTagDescriptionsManager().GetAvailableNames(GetClassName())).SetRequired(true);
    scheme.Add<TFSVariants>("class_name").SetVariants({ GetClassName() }).SetDefault(GetClassName()).SetRequired(true);
    scheme.Add<TFSString>("comments", "comments").SetRequired(true);
    return scheme;
}

NJson::TJsonValue ITag::SerializeToJson() const {
    NJson::TJsonValue jsonInfo;
    JWRITE(jsonInfo, "tag_name", Name);
    JWRITE(jsonInfo, "comments", Comments);
    return jsonInfo;
}

bool ITag::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
    JREAD_STRING(jsonInfo, "tag_name", Name);
    JREAD_STRING_OPT(jsonInfo, "comments", Comments);
    return true;
}
