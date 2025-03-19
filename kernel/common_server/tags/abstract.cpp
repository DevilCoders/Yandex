#include "abstract.h"

bool TTagStorageCustomization::WriteOldBinaryData = true;
bool TTagStorageCustomization::ReadOldBinaryData = true;
bool TTagStorageCustomization::WriteNewBinaryData = false;
bool TTagStorageCustomization::ReadNewBinaryData = false;
bool TTagStorageCustomization::WritePackedBinaryData = false;

const ITagDescriptions& ITagDescriptions::Instance() {
    return Singleton<TTagDescriptionsOperator>()->GetDescription();
}

NJson::TJsonValue ITagDescription::SerializeToJson() const {
    NJson::TJsonValue result = NJson::JSON_MAP;
    result.InsertValue("description", Description);
    result.InsertValue("unique_policy", ::ToString(GetUniquePolicy()));
    result.InsertValue("explicit_unique_policy", ::ToString(GetUniquePolicy()));
    return result;
}

bool ITagDescription::DeserializeFromJson(const NJson::TJsonValue& info) {
    if (!TJsonProcessor::Read(info, "description", Description)) {
        return false;
    }
    if (info.Has("explicit_unique_policy") && !TJsonProcessor::ReadFromString(info, "explicit_unique_policy", ExplicitUniquePolicy)) {
        return false;
    } else if (info.Has("unique_policy") && !TJsonProcessor::ReadFromString(info, "unique_policy", ExplicitUniquePolicy)) {
        return false;
    }
    if (!!ExplicitUniquePolicy && *ExplicitUniquePolicy == GetDefaultUniquePolicy()) {
        ExplicitUniquePolicy.Clear();
    }
    return true;
}

bool ITagDescription::DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values) {
    READ_DECODER_VALUE(decoder, values, Description);
    NJson::TJsonValue jsonValue;
    READ_DECODER_VALUE_JSON(decoder, values, jsonValue, TagDescriptionObject);
    if (!DeserializeFromJson(jsonValue)) {
        return false;
    }
    if (!decoder.GetValueMaybeAs(decoder.GetExplicitUniquePolicy(), values, ExplicitUniquePolicy)) {
        return false;
    }
    return true;
}

NCS::NStorage::TTableRecord ITagDescription::SerializeToTableRecord() const {
    NCS::NStorage::TTableRecord result;
    result.Set("tag_description_object", SerializeToJson().GetStringRobust());
    result.Set("description", Description);
    result.Set("explicit_unique_policy", ExplicitUniquePolicy.GetOrElse(GetDefaultUniquePolicy()));
    return result;
}

NFrontend::TScheme ITagDescription::GetScheme(const IBaseServer& /*server*/) const {
    NFrontend::TScheme result;
    result.Add<TFSString>("description");
    result.Add<TFSVariants>("explicit_unique_policy").InitVariants<EUniquePolicy>();
    return result;
}

bool TDBTagDescription::DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values) {
    if (!TBase::DeserializeWithDecoder(decoder, values)) {
        return false;
    }
    READ_DECODER_VALUE(decoder, values, Revision);
    READ_DECODER_VALUE(decoder, values, Name);
    READ_DECODER_VALUE(decoder, values, Deprecated);
    return true;
}

NJson::TJsonValue TDBTagDescription::SerializeToJson() const {
    NJson::TJsonValue result = TBase::SerializeToJson();
    result.InsertValue("name", Name);
    result.InsertValue("revision", Revision);
    result.InsertValue("deprecated", Deprecated);
    return result;
}

bool TDBTagDescription::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
    if (!TBase::DeserializeFromJson(jsonInfo)) {
        return false;
    }
    Name = jsonInfo["name"].GetStringSafe("");
    if (!Name) {
        return false;
    }
    if (!TJsonProcessor::Read(jsonInfo, "revision", Revision)) {
        return false;
    }
    if (jsonInfo.Has("deprecated")) {
        if (!jsonInfo["deprecated"].IsBoolean()) {
            return false;
        }
        Deprecated = jsonInfo["deprecated"].GetBooleanSafe(false);
    }
    return true;
}

NCS::NStorage::TTableRecord TDBTagDescription::SerializeToTableRecord() const {
    NCS::NStorage::TTableRecord result = TBase::SerializeToTableRecord();
    result.Set("name", Name);
    result.SetNotEmpty("revision", Revision);
    result.Set("deprecated", Deprecated);
    return result;
}

NFrontend::TScheme TDBTagDescription::GetScheme(const IBaseServer& server) {
    NFrontend::TScheme scheme = TBase::GetScheme(server);
    scheme.Add<TFSString>("name", "name").SetRequired(true);
    scheme.Add<TFSNumeric>("revision").SetRequired(false).SetReadOnly(true);
    scheme.Add<TFSBoolean>("deprecated", "is deprecated tag").SetDefault(false);
    return scheme;
}

void TTagDescriptionsOperator::Register(const ITagDescriptions* description) {
    CHECK_WITH_LOG(!Description);
    Description = description;
}

void TTagDescriptionsOperator::Unregister() {
    CHECK_WITH_LOG(Description);
    Description = nullptr;
}

const ITagDescriptions& TTagDescriptionsOperator::GetDescription() const {
    CHECK_WITH_LOG(Description);
    return *Description;
}
