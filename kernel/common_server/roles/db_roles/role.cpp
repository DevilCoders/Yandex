#include "role.h"

NJson::TJsonValue TDBRole::SerializeToJson() const {
    NJson::TJsonValue result = TBase::SerializeToJson();
    TJsonProcessor::Write(result, "role_id", RoleId);
    return result;
}

bool TDBRole::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
    if (!TBase::DeserializeFromJson(jsonInfo)) {
        return false;
    }
    if (!TJsonProcessor::Read(jsonInfo, "role_id", RoleId)) {
        return false;
    }
    return true;
}

NStorage::TTableRecord TDBRole::SerializeToTableRecord() const {
    NStorage::TTableRecord result;
    result.SetNotEmpty("role_id", RoleId);
    result.Set("role_name", RoleName);
    result.SetNotEmpty("revision", Revision);
    return result;
}

bool TDBRole::DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values) {
    READ_DECODER_VALUE(decoder, values, RoleId);
    READ_DECODER_VALUE(decoder, values, RoleName);
    READ_DECODER_VALUE(decoder, values, Revision);
    if (!RoleId || !RoleName) {
        return false;
    }
    return true;
}

NFrontend::TScheme TDBRole::GetScheme(const IBaseServer& server) {
    NFrontend::TScheme result = TBase::GetScheme(server);
    result.Add<TFSNumeric>("role_id").SetReadOnly(true);
    return result;
}
