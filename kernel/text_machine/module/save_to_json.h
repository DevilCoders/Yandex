#pragma once

#include <kernel/lingboost/enum_map.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/vec4/vec4.h>

#include <util/stream/str.h>
#include <util/string/cast.h>
#include <util/generic/vector.h>
#include <util/generic/ymath.h>

#include <array>

namespace NModule {
    class TJsonSerializable {
    public:
        void SaveToJson(NJson::TJsonValue&) {
        }
    };

    template <typename T>
    NJson::TJsonValue JsonValue(const T&);
    template <typename T>
    NJson::TJsonValue JsonFloat(T f);
    NJson::TJsonValue JsonVec4f(const TVec4f& v);
    template <typename T>
    NJson::TJsonValue JsonArray(const T& x);
    template <typename T>
    NJson::TJsonValue JsonEnumMap(const T& x);
    template <typename T>
    NJson::TJsonValue JsonClass(const T& x);

    namespace NPrivate {
        template <typename T, bool IsJsonSerializable>
        struct TJsonValueImpl {
            Y_FORCE_INLINE static NJson::TJsonValue Get(const T& x) {
                return NJson::TJsonValue(x);
            }
        };
        template <typename T>
        struct TJsonValueImpl<T, true> {
            Y_FORCE_INLINE static NJson::TJsonValue Get(const T& x) {
                return JsonClass(x);
            }
        };
        template <>
        struct TJsonValueImpl<double, false> {
            Y_FORCE_INLINE static NJson::TJsonValue Get(double x) {
                return JsonFloat(x);
            }
        };
        template <>
        struct TJsonValueImpl<float, false> {
            Y_FORCE_INLINE static NJson::TJsonValue Get(float x) {
                return JsonFloat(x);
            }
        };
        template <>
        struct TJsonValueImpl<TVec4f, false> {
            Y_FORCE_INLINE static NJson::TJsonValue Get(TVec4f x) {
                return JsonVec4f(x);
            }
        };
        template <typename T>
        struct TJsonValueImpl<TVector<T>, false> {
            Y_FORCE_INLINE static NJson::TJsonValue Get(const TVector<T>& x) {
                return JsonArray(x);
            }
        };
        template <typename T, size_t N>
        struct TJsonValueImpl<std::array<T, N>, false> {
            Y_FORCE_INLINE static NJson::TJsonValue Get(const std::array<T, N>& x) {
                return JsonArray(x);
            }
        };
        template <typename K, typename V>
        struct TJsonValueImpl<NLingBoost::TStaticEnumMap<K, V>, false> {
            Y_FORCE_INLINE static NJson::TJsonValue Get(const NLingBoost::TStaticEnumMap<K, V>& x) {
                return JsonEnumMap(x);
            }
        };
        template <typename K, typename V>
        struct TJsonValueImpl<NLingBoost::TPoolableEnumMap<K, V>, false> {
            Y_FORCE_INLINE static NJson::TJsonValue Get(const NLingBoost::TPoolableEnumMap<K, V>& x) {
                return JsonEnumMap(x);
            }
        };
        template <typename K, typename V>
        struct TJsonValueImpl<NLingBoost::TPoolableCompactEnumMap<K, V>, false> {
            Y_FORCE_INLINE static NJson::TJsonValue Get(const NLingBoost::TPoolableCompactEnumMap<K, V>& x) {
                return JsonEnumMap(x);
            }
        };
    }

    template <typename T>
    Y_FORCE_INLINE NJson::TJsonValue JsonValue(const T& x) {
        return NPrivate::TJsonValueImpl<T, std::is_base_of<TJsonSerializable, T>::value>::Get(x);
    }

    template <typename T>
    Y_FORCE_INLINE NJson::TJsonValue JsonFloat(T f) {
        if (IsValidFloat(f)) {
            return NJson::TJsonValue(f);
        } else {
            return NJson::TJsonValue("NAN");
        }
    }
    Y_FORCE_INLINE NJson::TJsonValue JsonVec4f(const TVec4f& v) {
        TStringStream out;
        out << v;
        return NJson::TJsonValue(out.Str());
    }
    template <typename T>
    Y_FORCE_INLINE NJson::TJsonValue JsonArray(const T& x) {
        NJson::TJsonValue value;
        for (size_t i = 0; i != x.size(); ++i) {
            value[i] = JsonValue(x[i]);
        }
        return value;
    }
    template <typename T>
    Y_FORCE_INLINE NJson::TJsonValue JsonEnumMap(const T& x) {
        NJson::TJsonValue value;
        for (auto entry : x) {
            value[ToString(entry.Key())] = JsonValue(entry.Value());
        }
        return value;
    }
    template <typename T>
    Y_FORCE_INLINE NJson::TJsonValue JsonClass(const T& x) {
        NJson::TJsonValue value;
        x.SaveToJson(value);
        return value;
    }
} // NModule
