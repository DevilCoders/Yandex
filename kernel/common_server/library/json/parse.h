#pragma once

#include "cast.h"

namespace NJson {
    template <class T>
    void InsertField(TJsonValue& dict, TStringBuf key, T&& value) {
        Y_ASSERT(dict.IsMap() || !dict.IsDefined());
        Y_ASSERT(!dict.Has(key));
        dict[key] = NJson::ToJson(std::forward<T>(value));
    }

    template <class T>
    void InsertNonDefault(TJsonValue& dict, TStringBuf key, T&& value, const T& def) {
        if (value != def) {
            InsertField(dict, key, std::forward<T>(value));
        }
    }

    template <class T>
    void InsertNonNull(TJsonValue& dict, TStringBuf key, T&& value) {
        if (value) {
            InsertField(dict, key, std::forward<T>(value));
        }
    }

    template <class T>
    bool ParseField(const TJsonValue& field, T&& value, bool required = false) {
        if (!field.IsDefined()) {
            return !required;
        }
        return TryFromJson(field, std::forward<T>(value));
    }

    template <class T>
    bool ParseField(const TJsonValue& dict, TStringBuf key, T&& value, bool required = false) {
        return ParseField(dict[key], std::forward<T>(value), required);
    }

    template <class T>
    void ReadField(const TJsonValue& dict, TStringBuf key, T&& value, bool required = false) {
        if (!ParseField(dict, key, std::forward<T>(value), required)) {
            throw yexception() << "cannot read field " << key << " from " << dict[key].GetStringRobust();
        }
    }
}
