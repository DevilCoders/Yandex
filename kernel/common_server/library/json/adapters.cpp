#include "adapters.h"

#include <library/cpp/json/json_reader.h>

#include <util/string/hex.h>
#include <util/string/strip.h>

NJson::NPrivate::THexString::THexString(const void* data, size_t size)
    : ConstData(data)
    , MutableData(nullptr)
    , Size(size)
{
}

NJson::NPrivate::THexString::THexString(void* data, size_t size)
    : ConstData(data)
    , MutableData(data)
    , Size(size)
{
}

NJson::NPrivate::TJsonString NJson::JsonString(TStringBuf object) {
    return { object };
}

template <>
NJson::TJsonValue NJson::ToJson(const NJson::NPrivate::THexString& object) {
    return HexEncode(object.ConstData, object.Size);
}

NJson::TJsonValue NJson::ToJson(const NJson::NPrivate::TJsonString& object) {
    auto value = object.Value;
    if (value.empty()) {
        return value;
    }
    auto stripped = StripString<TStringBuf>(value);
    if (
        (stripped.StartsWith('{') && stripped.EndsWith('}')) ||
        (stripped.StartsWith('[') && stripped.EndsWith(']')) ||
        (stripped.StartsWith('"') && stripped.EndsWith('"'))
    ) {
        NJson::TJsonValue parsed;
        if (NJson::ReadJsonFastTree(stripped, &parsed)) {
            return parsed;
        }
    }
    return value;
}

bool NJson::TryFromJson(const NJson::TJsonValue& value, NJson::NPrivate::THexString&& result) {
    TString hexString;
    if (!NJson::TryFromJson(value, hexString)) {
        return false;
    }
    if (hexString.size() % 2) {
        return false;
    }
    if (hexString.size() / 2 != result.Size) {
        return false;
    }
    return HexDecode(hexString.data(), hexString.size(), result.MutableData) != nullptr;
}
