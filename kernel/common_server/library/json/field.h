#pragma once

#include "parse.h"

#include <kernel/common_server/util/types/field.h>

namespace NJson {
    namespace NPrivate {
        template <class T>
        TJsonValue FieldsToJson(T&& fields) {
            TJsonValue result = JSON_MAP;
            ForEach(std::forward<T>(fields), [&result](auto&& field) {
                Y_ASSERT(field.HasName());
                InsertField(result, field.GetName(), field.Value);
            });
            return result;
        }

        template <class T>
        bool TryFieldsFromJson(const TJsonValue& value, T&& fields) {
            bool result = true;
            ForEach(std::forward<T>(fields), [&result, &value](auto&& field) {
                Y_ASSERT(field.HasName());
                if (result) {
                    result = ParseField(value[field.GetName()], field.Value);
                }
            });
            return result;
        }
    }

    template <class T>
    TJsonValue FieldsToJson(const T& object) {
        return NPrivate::FieldsToJson(object.GetFields());
    }
    template <class T>
    bool TryFieldsFromJson(const TJsonValue& value, T& result) {
        return NPrivate::TryFieldsFromJson(value, result.GetFields());
    }

    template <class T>
    TJsonValue ToJson(const TField<T>& object) {
        TJsonValue result;
        result["value"] = NJson::ToJson(object.Value);
        if (object.HasName()) {
            result["name"] = object.GetName();
        }
        if (object.HasIndex()) {
            result["index"] = object.GetIndex();
        }
        return result;
    }
}

#define DECLARE_FIELDS_JSON_SERIALIZER(T)                                   \
    template <>                                                             \
    NJson::TJsonValue NJson::ToJson(const T& object) {                      \
        return NJson::FieldsToJson(object);                                 \
    }                                                                       \
    template <>                                                             \
    bool NJson::TryFromJson(const NJson::TJsonValue& value, T& result) {    \
        return NJson::TryFieldsFromJson(value, result);                     \
    }
