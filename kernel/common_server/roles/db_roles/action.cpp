#include "action.h"

NFrontend::TScheme TDBItemPermissions::GetScheme(const IBaseServer& server) {
    auto result = TBase::GetScheme(server);
    return result;
}

NJson::TJsonValue TDBItemPermissions::SerializeToJson() const {
    NJson::TJsonValue result = TBase::SerializeToJson();
    return result;
}

bool TDBItemPermissions::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
    if (!TBase::DeserializeFromJson(jsonInfo)) {
        return false;
    }
    return true;
}

NStorage::TTableRecord TDBItemPermissions::SerializeToTableRecord() const {
    NStorage::TTableRecord result = TBase::SerializeToTableRecord();
    result.Set("item_id", ItemId);
    result.Set("revision", Revision);
    return result;
}

bool TDBItemPermissions::DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values) {
    if (!TBase::DeserializeWithDecoder(decoder, values)) {
        return false;
    }
    READ_DECODER_VALUE(decoder, values, ItemId);
    READ_DECODER_VALUE(decoder, values, Revision);
    if (!ItemId) {
        return false;
    }
    return true;
}
