#include "cast.h"

#include <kernel/common_server/util/types/timestamp.h>

#include <util/datetime/base.h>
#include <util/string/cast.h>

#include <cmath>

// signed integer types
#define DEFINE_JSON_CAST(T)                                       \
    template <>                                                   \
    NJson::TJsonValue NJson::ToJson(const T& object) {            \
        return object;                                            \
    }                                                             \
    template <>                                                   \
    bool NJson::TryFromJson(const TJsonValue& value, T& result) { \
        if (value.IsInteger()) {                                  \
            auto v = value.GetInteger();                          \
            if (v > std::numeric_limits<T>::max()) {              \
                return false;                                     \
            }                                                     \
            if (v < std::numeric_limits<T>::min()) {              \
                return false;                                     \
            }                                                     \
            result = v;                                           \
            return true;                                          \
        }                                                         \
        if (value.IsString()) {                                   \
            return TryFromString(value.GetString(), result);      \
        }                                                         \
        return false;                                             \
    }

DEFINE_JSON_CAST(i8)
DEFINE_JSON_CAST(i16)
DEFINE_JSON_CAST(i32)
DEFINE_JSON_CAST(i64)

#undef DEFINE_JSON_CAST

// unsigned integer types
#define DEFINE_JSON_CAST(T)                                       \
    template <>                                                   \
    NJson::TJsonValue NJson::ToJson(const T& object) {            \
        return object;                                            \
    }                                                             \
    template <>                                                   \
    bool NJson::TryFromJson(const TJsonValue& value, T& result) { \
        if (value.IsUInteger()) {                                 \
            auto v = value.GetUInteger();                         \
            if (v > std::numeric_limits<T>::max()) {              \
                return false;                                     \
            }                                                     \
            if (v < std::numeric_limits<T>::min()) {              \
                return false;                                     \
            }                                                     \
            result = v;                                           \
            return true;                                          \
        }                                                         \
        if (value.IsString()) {                                   \
            return TryFromString(value.GetString(), result);      \
        }                                                         \
        return false;                                             \
    }

DEFINE_JSON_CAST(ui8)
DEFINE_JSON_CAST(ui16)
DEFINE_JSON_CAST(ui32)
DEFINE_JSON_CAST(ui64)

#undef DEFINE_JSON_CAST

// float types
#define DEFINE_JSON_CAST(T)                                       \
    template <>                                                   \
    NJson::TJsonValue NJson::ToJson(const T& object) {            \
        return object;                                            \
    }                                                             \
    template <>                                                   \
    bool NJson::TryFromJson(const TJsonValue& value, T& result) { \
        if (value.IsDouble()) {                                   \
            auto v = value.GetDouble();                           \
            if (std::abs(v) > std::numeric_limits<T>::max()) {    \
                return false;                                     \
            }                                                     \
            result = v;                                           \
            return true;                                          \
        }                                                         \
        if (value.IsString()) {                                   \
            return TryFromString(value.GetString(), result);      \
        }                                                         \
        return false;                                             \
    }

DEFINE_JSON_CAST(float)
DEFINE_JSON_CAST(double)

#undef DEFINE_JSON_CAST

// string types
#define DEFINE_JSON_CAST(T)                                       \
    template <>                                                   \
    NJson::TJsonValue NJson::ToJson(const T& object) {            \
        return object;                                            \
    }                                                             \
    template <>                                                   \
    bool NJson::TryFromJson(const TJsonValue& value, T& result) { \
        if (!value.IsString()) {                                  \
            return false;                                         \
        }                                                         \
        result = value.GetString();                               \
        return true;                                              \
    }

DEFINE_JSON_CAST(TString)
DEFINE_JSON_CAST(TStringBuf)

#undef DEFINE_JSON_CAST

template <>
NJson::TJsonValue NJson::ToJson(const bool& object) {
    return object;
}

template <>
bool NJson::TryFromJson(const TJsonValue& value, bool& result) {
    if (value.IsBoolean()) {
        result = value.GetBoolean();
        return true;
    }
    if (value.IsUInteger()) {
        auto v = value.GetUInteger();
        result = v;
        return true;
    }
    if (value.IsString()) {
        const TString& v = value.GetString();
        return TryFromString(v, result);
    }
    return false;
}

template <>
NJson::TJsonValue NJson::ToJson(const TInstant& object) {
    return object.MicroSeconds();
}

template <>
bool NJson::TryFromJson(const TJsonValue& value, TInstant& result) {
    if (value.IsUInteger()) {
        auto v = value.GetUInteger();
        if (v > TInstant::Seconds(1000000000).MicroSeconds()) {
            result = TInstant::MicroSeconds(v);
        } else {
            result = TInstant::Seconds(v);
        }
        return true;
    }
    if (value.IsString()) {
        if (TInstant::TryParseIso8601(value.GetString(), result)) {
            return true;
        }
    }
    if (value.IsDouble()) {
        auto v = value.GetDouble();
        if (v > 0) {
            result = TInstant::Seconds(v);
            return true;
        }
    }
    return false;
}

template <>
NJson::TJsonValue NJson::ToJson(const TDuration& object) {
    return object.MicroSeconds();
}

template <>
bool NJson::TryFromJson(const TJsonValue& value, TDuration& result) {
    if (value.IsUInteger()) {
        result = TDuration::MicroSeconds(value.GetUInteger());
        return true;
    }
    if (value.IsString()) {
        return TryFromString(value.GetString(), result);
    }
    if (value.IsDouble()) {
        auto v = value.GetDouble();
        if (v > 0) {
            result = TDuration::Seconds(v);
            return true;
        }
    }
    return false;
}

template <>
NJson::TJsonValue NJson::ToJson(const TTimestamp& object) {
    return object.Seconds();
}

template <>
bool NJson::TryFromJson(const TJsonValue& value, TTimestamp& result) {
    if (value.IsUInteger()) {
        result = TInstant::Seconds(value.GetUInteger());
        return true;
    }
    if (value.IsString()) {
        TInstant v;
        if (TInstant::TryParseIso8601(value.GetString(), v)) {
            result = v;
            return true;
        }
    }
    return false;
}

template <>
NJson::TJsonValue NJson::ToJson(const TSeconds& object) {
    return object.Seconds();
}

template <>
bool NJson::TryFromJson(const TJsonValue& value, TSeconds& result) {
    if (value.IsUInteger()) {
        result = TDuration::Seconds(value.GetUInteger());
        return true;
    }
    if (value.IsString()) {
        TDuration v;
        if (TryFromString(value.GetString(), v)) {
            result = v;
            return true;
        }
    }
    return false;
}

template <>
NJson::TJsonValue NJson::ToJson(const NJson::TJsonValue& object) {
    return object;
}

template <>
bool NJson::TryFromJson(const NJson::TJsonValue& value, NJson::TJsonValue& result) {
    result = value;
    return true;
}
