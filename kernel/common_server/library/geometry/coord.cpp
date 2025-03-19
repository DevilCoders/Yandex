#include "coord.h"

#include <kernel/common_server/library/json/cast.h>

#include <tuple>

template <>
bool TryFromStringImpl<TCoord<double>>(const char* data, size_t len, TCoord<double>& value) {
    TStringBuf s(data, len);
    return value.DeserializeFromStringLonLat(s);
}

template <>
bool TryFromStringImpl<TGeoCoord>(const char* data, size_t len, TGeoCoord& value) {
    TStringBuf s(data, len);
    return value.DeserializeFromStringLonLat(s);
}

template <>
TGeoCoord FromStringImpl(const char* data, size_t len) {
    TStringBuf s(data, len);
    return TGeoCoord(s);
}

template <>
void Out<TGeoCoord>(IOutputStream& out, const TGeoCoord& value) {
    out << value.X << " " << value.Y;
}

template <>
bool NJson::TryFromJson(const NJson::TJsonValue& value, TGeoCoord& result) {
    return NJson::TryFromJson(value, std::tie(result.X, result.Y));
}

template <>
NJson::TJsonValue NJson::ToJson(const TGeoCoord& object) {
    return object.SerializeToJson();
}
