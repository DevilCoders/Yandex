#pragma once

#include "cast.h"

#include <util/string/cast.h>

namespace NJson {
    namespace NPrivate {
        template <class T>
        struct TDictionary {
        public:
            TDictionary(T& value)
                : Value(value)
            {
            }

        public:
            T& Value;
        };

        struct THexString {
        public:
            THexString(const void* data, size_t size);
            THexString(void* data, size_t size);

        public:
            const void* ConstData;
            void* MutableData;
            size_t Size;
        };

        struct TJsonString {
        public:
            TJsonString(TStringBuf value)
                : Value(value)
            {
            }

        public:
            TStringBuf Value;
        };

        template <class T>
        struct TKeyValue {
        public:
            TKeyValue(T& container, TStringBuf key = "key", TStringBuf value = "value")
                : Container(container)
                , Key(key)
                , Value(value)
            {
            }

        public:
            T& Container;
            TStringBuf Key;
            TStringBuf Value;
        };

        template <class T>
        struct TNullable {
        public:
            TNullable(const T& value)
                : Value(value)
            {
            }

        public:
            const T& Value;
        };

        template <class T>
        struct TStringify {
        public:
            TStringify(T& value)
                : Value(value)
            {
            }

        public:
            T& Value;
        };
    }

    template <class T>
    NPrivate::TDictionary<T> Dictionary(T& object) {
        return { object };
    }

    template <class T>
    NPrivate::THexString HexString(T& object) {
        return { std::data(object), std::size(object) * sizeof(*std::data(object)) };
    }

    NPrivate::TJsonString JsonString(TStringBuf object);

    template <class T>
    NPrivate::TKeyValue<T> KeyValue(T& object, TStringBuf key = "key", TStringBuf value = "value") {
        return { object, key, value };
    }

    template <class T>
    NPrivate::TNullable<T> Nullable(const T& object) {
        return { object };
    }

    template <class T>
    NPrivate::TStringify<T> Stringify(T& object) {
        return { object };
    }
}

namespace NJson {
    template <class T>
    TJsonValue ToJson(const NPrivate::TDictionary<T>& object) {
        TJsonValue result = JSON_MAP;
        for (auto&&[key, value] : object.Value) {
            result[ToString(key)] = ToJson(value);
        }
        return result;
    }

    TJsonValue ToJson(const NPrivate::TJsonString& object);

    template <class T>
    TJsonValue ToJson(const NPrivate::TKeyValue<T>& object) {
        TJsonValue result = JSON_ARRAY;
        for (auto&&[key, value] : object.Container) {
            TJsonValue element;
            element.InsertValue(object.Key, ToJson(key));
            element.InsertValue(object.Value, ToJson(value));
            result.AppendValue(std::move(element));
        }
        return result;
    }

    template <class T>
    TJsonValue ToJson(const NPrivate::TNullable<T>& object) {
        if (object.Value) {
            return ToJson(object.Value);
        } else {
            return NJson::JSON_NULL;
        }
    }

    template <class T>
    TJsonValue ToJson(const NPrivate::TStringify<T>& object) {
        return ToString(object.Value);
    }

    template <class T>
    bool TryFromJson(const NJson::TJsonValue& value, NPrivate::TDictionary<T>&& result) {
        if (!value.IsMap()) {
            return false;
        }
        for (auto&&[k, v] : value.GetMap()) {
            typename T::key_type key;
            typename T::mapped_type value;
            if (!TryFromString(k, key) || !TryFromJson(v, value)) {
                return false;
            }
            result.Value.emplace(std::move(key), std::move(value));
        }
        return true;
    }

    bool TryFromJson(const NJson::TJsonValue& value, NPrivate::THexString&& result);

    template <class T>
    bool TryFromJson(const NJson::TJsonValue& value, NPrivate::TKeyValue<T>&& result) {
        if (!value.IsArray()) {
            return false;
        }
        for (auto&& element : value.GetArray()) {
            typename T::key_type key;
            typename T::mapped_type value;
            if (!TryFromJson(element[result.Key], key) || !TryFromJson(element[result.Value], value)) {
                return false;
            }
            result.Container.emplace(std::move(key), std::move(value));
        }
        return true;
    }

    template <class T>
    bool TryFromJson(const NJson::TJsonValue& value, NPrivate::TStringify<T>&& result) {
        TString s;
        return TryFromJson(value, s) && TryFromString(s, result.Value);
    }
}
