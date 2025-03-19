#include "rect.h"

#include <kernel/common_server/library/json/cast.h>

#include <tuple>

template <>
void Out<TGeoRect>(IOutputStream& out, const TGeoRect& value) {
    out
        << value.Min.X << ' '
        << value.Min.Y << ' '
        << value.Max.X << ' '
        << value.Max.Y;
}

template <>
TGeoRect FromStringImpl(const char* data, size_t len) {
    TStringBuf s(data, len);
    TStringBuf token;
    TGeoRect value;
    Y_ENSURE(s.NextTok(' ', token));
    Y_ENSURE(TryFromString(token, value.Min.X));
    Y_ENSURE(s.NextTok(' ', token));
    Y_ENSURE(TryFromString(token, value.Min.Y));
    Y_ENSURE(s.NextTok(' ', token));
    Y_ENSURE(TryFromString(token, value.Max.X));
    Y_ENSURE(s.NextTok(' ', token));
    Y_ENSURE(TryFromString(token, value.Max.Y));
    return value;
}

template <>
bool TryFromStringImpl(const char* data, size_t len, TGeoRect& value) {
    TStringBuf s(data, len);
    TStringBuf token;
    return
        s.NextTok(' ', token) && TryFromString(token, value.Min.X) &&
        s.NextTok(' ', token) && TryFromString(token, value.Min.Y) &&
        s.NextTok(' ', token) && TryFromString(token, value.Max.X) &&
        s.NextTok(' ', token) && TryFromString(token, value.Max.Y);
}

template <>
NJson::TJsonValue NJson::ToJson(const TGeoRect& object) {
    return NJson::ToJson(std::tie(object.Min, object.Max));
}

template <>
bool NJson::TryFromJson(const NJson::TJsonValue& value, TGeoRect& result) {
    return NJson::TryFromJson(value, std::tie(result.Min, result.Max));
}
