#pragma once

#include "accessor.h"
#include <iterator>
#include <kernel/common_server/library/logging/events.h>
#include <library/cpp/charset/ci_string.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/json/writer/json_value.h>
#include <library/cpp/logger/global/global.h>
#include <util/generic/maybe.h>
#include <util/generic/serialized_enum.h>
#include <util/generic/set.h>
#include <util/string/join.h>
#include <util/string/split.h>
#include <util/system/yassert.h>

class TJsonProcessor {
public:
    static const NJson::TJsonValue& GetAvailableElement(const NJson::TJsonValue& jsonBase, const TVector<TString>& children);

    static TString FormatDurationString(const TDuration value);
    template <class T>
    static TString AlignString(const T& value, const size_t size, const char additional = ' ') {
        TString result = ::ToString(value);
        result.resize(size, additional);
        return result;
    }
    static void WriteDurationInt(NJson::TJsonValue& target, const TString& fieldName, const TDuration value, const TMaybe<TDuration> defValue = {});
    static void WriteInstant(NJson::TJsonValue& target, const TString& fieldName, const TInstant value, const TMaybe<TInstant> defValue = {});

    template <class T>
    static NJson::TJsonValue WriteMapSimple(const TMap<TString, T>& values) {
        NJson::TJsonValue result = NJson::JSON_MAP;
        for (auto&& i : values) {
            result.InsertValue(i.first, i.second);
        }
        return result;
    }

    template <class T>
    static void WriteMapSimple(NJson::TJsonValue& target, const TString& fieldName, const TMap<TString, T>& values) {
        target.InsertValue(fieldName, WriteMapSimple(values));
    }

    template <class T>
    Y_WARN_UNUSED_RESULT static bool ReadMapSimple(const NJson::TJsonValue& jsonInfo, TMap<TString, T>& result) {
        const NJson::TJsonValue::TMapType* jsonMap = nullptr;
        if (!jsonInfo.GetMapPointer(&jsonMap)) {
            TFLEventLog::Error("incorrect_field_type")("expected", "map")("actual", jsonInfo.GetType());
            return false;
        }
        TMap<TString, T> localResult;
        for (auto&& i : *jsonMap) {
            T value;
            if (!Read(jsonInfo, i.first, value, true)) {
                TFLEventLog::Error("cannot parse map value")("raw_info", jsonInfo.GetStringRobust());
                return false;
            }
            localResult.emplace(i.first, value);
        }
        std::swap(localResult, result);
        return true;
    }

    template <class T>
    Y_WARN_UNUSED_RESULT static bool ReadMapSimple(const NJson::TJsonValue& jsonInfo, const TString& fieldName, TMap<TString, T>& values, const bool mustBe = false) {
        if (!jsonInfo.Has(fieldName)) {
            if (mustBe) {
                TFLEventLog::Error("no_field")("field", fieldName);
            }
            return !mustBe;
        } else {
            return ReadMapSimple(jsonInfo[fieldName], values);
        }
    }

    template <class T>
    static void WriteAsString(NJson::TJsonValue& target, const TString& fieldName, const T& value) {
        Write(target, fieldName, ::ToString(value));
    }

    template <class T>
    static void WriteDef(NJson::TJsonWriter& writer, const TString& fieldName, const T& value, const T defValue) {
        if (value == defValue) {
            return;
        }
        writer.Write(fieldName, value);
    }

    template <class T>
    static void WriteDef(NJson::TJsonValue& writer, const TString& fieldName, const T& value, const T defValue) {
        if (value == defValue) {
            return;
        }
        writer.InsertValue(fieldName, value);
    }

    template <class TMaybePolicy>
    static void WriteInstant(NJson::TJsonValue& target, const TString& fieldName, const TMaybe<TInstant, TMaybePolicy> value, const TMaybe<TInstant> defValue = {}) {
        if (value) {
            WriteInstant(target, fieldName, *value, defValue);
        }
    }

    static void WriteDurationString(NJson::TJsonValue& target, const TString& fieldName, const TDuration value, const TMaybe<TDuration> defValue = {});

    template <class TMaybePolicy>
    static void WriteDurationString(NJson::TJsonValue& target, const TString& fieldName, const TMaybe<TDuration, TMaybePolicy> value, const TMaybe<TDuration> defValue = {}) {
        if (!value) {
            return;
        }
        WriteDurationString(target, fieldName, *value, defValue);
    }

    template <class T>
    Y_WARN_UNUSED_RESULT static bool ReadFromString(const NJson::TJsonValue& target, const TString& fieldName, T& result, const bool mustBe = false) {
        if (!mustBe && !target.Has(fieldName)) {
            return true;
        }
        TString resultStr;
        return Read(target, fieldName, resultStr, mustBe) && TryFromString(resultStr, result);
    }

    template <class Enum, class TEnumSet>
    static void WriteEnumSet(NJson::TJsonValue& jsonInfo, const TString& fieldName, const TEnumSet value) {
        auto& jsonArray = jsonInfo.InsertValue(fieldName, NJson::JSON_ARRAY);
        for (auto&& i : GetEnumAllValues<Enum>()) {
            if (value & (TEnumSet)i) {
                jsonArray.AppendValue(::ToString(i));
            }
        }
    }

    template <class Enum, class TEnumSet>
    Y_WARN_UNUSED_RESULT static bool ReadEnumSet(const NJson::TJsonValue& jsonInfo, TEnumSet& result) {
        auto gLogging = TFLRecords::StartContext()("raw_data", jsonInfo);
        const NJson::TJsonValue::TArray* arr = nullptr;
        if (!jsonInfo.GetArrayPointer(&arr)) {
            TFLEventLog::Error("incorrect enum set type")("reason", "not array");
            return false;
        }
        TEnumSet resultInternal = 0;
        for (auto&& i : *arr) {
            if (!i.IsString()) {
                TFLEventLog::Error("incorrect enum set element type")("reason", "not string");
                return false;
            }
            Enum e;
            if (!TryFromString<Enum>(i.GetString(), e)) {
                TFLEventLog::Error("incorrect enum set element type")("reason", "cannot cast to enum");
                return false;
            }
            resultInternal |= (TEnumSet)e;
        }
        std::swap(result, resultInternal);
        return true;
    }

    template <class Enum, class TEnumSet>
    Y_WARN_UNUSED_RESULT static bool ReadEnumSet(const NJson::TJsonValue& jsonInfo, const TString& fieldName, TEnumSet& result, const bool mustBe = false) {
        if (jsonInfo.Has(fieldName)) {
            return ReadEnumSet<Enum, TEnumSet>(jsonInfo[fieldName], result);
        }
        return !mustBe;
    }

    template <class T, class TMaybePolicy>
    Y_WARN_UNUSED_RESULT static bool ReadFromString(const NJson::TJsonValue& jsonInfo, const TString& fieldName, TMaybe<T, TMaybePolicy>& result, const bool mustBe = false) {
        if (jsonInfo.Has(fieldName)) {
            T resultLocal;
            if (!ReadFromString(jsonInfo, fieldName, resultLocal, mustBe)) {
                return false;
            }
            result = resultLocal;
            return true;
        }
        if (mustBe) {
            TFLEventLog::Error("no_field")("field", fieldName);
        }
        return !mustBe;
    }

    Y_WARN_UNUSED_RESULT static bool Read(const NJson::TJsonValue& jsonInfo, const TString& fieldName, TDuration& result, const bool mustBe = false);

    template <class T>
    Y_WARN_UNUSED_RESULT static bool Read(const NJson::TJsonValue& jsonInfo, const TString& fieldName, TMap<TString, T>& result, const bool mustBe = false) {
        auto gLogging = TFLRecords::StartContext()("field_name", fieldName);
        const NJson::TJsonValue::TArray* arrRemapInfo = nullptr;
        if (jsonInfo.Has(fieldName)) {
            if (jsonInfo[fieldName].GetArrayPointer(&arrRemapInfo)) {
                for (auto&& i : *arrRemapInfo) {
                    TString fromStr;
                    T toVal;
                    if (!Read(i, "from", fromStr, true)) {
                        TFLEventLog::Error("cannot read key")("raw_data", i);
                        return false;
                    }
                    if (!Read(i, "to", toVal, true)) {
                        TFLEventLog::Error("cannot read value")("raw_data", i);
                        return false;
                    }
                    if (!result.emplace(fromStr, std::move(toVal)).second) {
                        TFLEventLog::Error("cannot insert into result map")("reason", "key duplication")("raw_data", i);
                        return false;
                    }
                }
                return true;
            } else {
                TFLEventLog::Error("incorrect array type");
                return false;
            }
        } else {
            if (mustBe) {
                TFLEventLog::Error("no_field")("field", fieldName);
            }
            return !mustBe;
        }
    }

    Y_WARN_UNUSED_RESULT static bool Read(const NJson::TJsonValue& jsonInfo, const TString& fieldName, double& result, const bool mustBe = false) {
        const NJson::TJsonValue* jsonValue;
        if (!jsonInfo.GetValuePointer(fieldName, &jsonValue)) {
            TFLEventLog::Error("no_field")("field", fieldName);
            return !mustBe;
        }
        if (jsonValue->IsDouble()) {
            result = jsonValue->GetDouble();
            return true;
        }
        TFLEventLog::Error("incorrect_field_type")("field", fieldName)("type", jsonValue->GetType());
        return false;
    }

    Y_WARN_UNUSED_RESULT static bool Read(const NJson::TJsonValue& jsonInfo, const TString& fieldName, ui32& result, const bool mustBe = false, const bool canBeZero = true);
    Y_WARN_UNUSED_RESULT static bool Read(const NJson::TJsonValue& jsonInfo, const TString& fieldName, ui64& result, const bool mustBe = false, const bool canBeZero = true);
    Y_WARN_UNUSED_RESULT static bool Read(const NJson::TJsonValue& jsonInfo, const TString& fieldName, i32& result, const bool mustBe = false, const bool canBeZero = true);
    Y_WARN_UNUSED_RESULT static bool Read(const NJson::TJsonValue& jsonInfo, const TString& fieldName, i64& result, const bool mustBe = false, const bool canBeZero = true);
    Y_WARN_UNUSED_RESULT static bool Read(const NJson::TJsonValue& jsonInfo, const TString& fieldName, TString& result, const bool mustBe = false, const bool mayBeEmpty = true);
    Y_WARN_UNUSED_RESULT static bool Read(const NJson::TJsonValue& jsonInfo, const TString& fieldName, bool& result, const bool mustBe = false);

    template <class TMapContainer>
    Y_WARN_UNUSED_RESULT static bool ReadMap(const NJson::TJsonValue& jsonInfo, const TString& fieldName, TMapContainer& result, const bool mustBe = false) {
        if (!jsonInfo.Has(fieldName)) {
            return !mustBe;
        }
        const NJson::TJsonValue::TArray* arr = nullptr;
        if (!jsonInfo[fieldName].GetArrayPointer(&arr)) {
            TFLEventLog::Error("incorrect json field")("field", fieldName)("type", jsonInfo[fieldName].GetType());
            return false;
        }
        TMapContainer resultLocal;
        for (auto&& i : *arr) {
            if (!i.Has("key")) {
                TFLEventLog::Error("not found key field for map deserialization")("raw_data", jsonInfo[fieldName]);
                return false;
            }
            if (!i.Has("value")) {
                TFLEventLog::Error("not found key field for map deserialization")("raw_data", jsonInfo[fieldName]);
                return false;
            }
            const TString& key = i["key"].GetStringRobust();
            const TString& value = i["value"].GetStringRobust();
            typename TMapContainer::mapped_type v;
            if (!TryFromString(value, v)) {
                TFLEventLog::Error("incorrect value type")("raw_data", jsonInfo[fieldName]);
                return false;
            }
            const size_t sizeBefore = resultLocal.size();
            resultLocal.emplace(key, v);
            if (resultLocal.size() == sizeBefore) {
                TFLEventLog::Error("cannot insert key")("raw_data", jsonInfo[fieldName]);
                return false;
            }
        }
        std::swap(resultLocal, result);
        return true;
    }

    template <class T>
    Y_WARN_UNUSED_RESULT static bool ReadObjectsMap(const NJson::TJsonValue& jsonInfo, const TString& fieldName, TMap<TString, T>& result, const bool mustBe = false) {
        if (!jsonInfo.Has(fieldName)) {
            return !mustBe;
        }
        const NJson::TJsonValue::TArray* arr = nullptr;
        if (!jsonInfo[fieldName].GetArrayPointer(&arr)) {
            TFLEventLog::Error("incorrect json field")("field", fieldName)("type", jsonInfo[fieldName].GetType());
            return false;
        }
        for (auto&& i : *arr) {
            if (!i.Has("key")) {
                TFLEventLog::Error("not found key field for map deserialization")("raw_data", jsonInfo[fieldName]);
                return false;
            }
            if (!i.Has("value")) {
                TFLEventLog::Error("not found key field for map deserialization")("raw_data", jsonInfo[fieldName]);
                return false;
            }
            const TString& key = i["key"].GetStringRobust();
            T v;
            if (!v.DeserializeFromJson(i["value"])) {
                TFLEventLog::Error("incorrect value type")("raw_data", jsonInfo[fieldName]);
                return false;
            }
            if (!result.emplace(key, std::move(v)).second) {
                TFLEventLog::Error("map keys duplication")("raw_data", jsonInfo[fieldName]);
                return false;
            }
        }
        return true;
    }

    template <class T>
    Y_WARN_UNUSED_RESULT static bool ReadObjectsMapSimple(const NJson::TJsonValue& jsonInfo, const TString& fieldName, TMap<TString, T>& result, const bool mustBe = false) {
        if (!jsonInfo.Has(fieldName)) {
            return !mustBe;
        }
        const NJson::TJsonValue::TMapType* jsonMap = nullptr;
        if (!jsonInfo[fieldName].GetMapPointer(&jsonMap)) {
            TFLEventLog::Error("incorrect json field")("field", fieldName)("type", jsonInfo[fieldName].GetType());
            return false;
        }
        for (auto&& [key, value] : *jsonMap) {
            T v;
            if (!v.DeserializeFromJson(value)) {
                TFLEventLog::Error("incorrect value type")("raw_data", jsonInfo[fieldName]);
                return false;
            }
            if (!result.emplace(key, std::move(v)).second) {
                TFLEventLog::Error("map keys duplication")("raw_data", jsonInfo[fieldName]);
                return false;
            }
        }
        return true;
    }

    template <class T>
    static void WriteObjectsMapSimple(NJson::TJsonValue& target, const TString& fieldName, const TMap<TString, T>& values) {
        NJson::TJsonValue& jsonMap = target.InsertValue(fieldName, NJson::JSON_MAP);
        for (auto&& [key, value] : values) {
            jsonMap.InsertValue(key, value.SerializeToJson());
        }
    }

    template <class TMapContainer>
    static void WriteMap(NJson::TJsonValue& target, const TString& fieldName, const TMapContainer& values) {
        NJson::TJsonValue& jsonArr = target.InsertValue(fieldName, NJson::JSON_ARRAY);
        for (auto&& i : values) {
            NJson::TJsonValue& jsonItem = jsonArr.AppendValue(NJson::JSON_MAP);
            jsonItem.InsertValue("key", i.first);
            jsonItem.InsertValue("value", i.second);
        }
    }

    template <class T>
    static void WriteObjectsMap(NJson::TJsonValue& target, const TString& fieldName, const TMap<TString, T>& values) {
        NJson::TJsonValue& jsonArr = target.InsertValue(fieldName, NJson::JSON_ARRAY);
        for (auto&& i : values) {
            NJson::TJsonValue& jsonItem = jsonArr.AppendValue(NJson::JSON_MAP);
            jsonItem.InsertValue("key", i.first);
            jsonItem.InsertValue("value", i.second.SerializeToJson());
        }
    }

    template <class T, class TMaybePolicy>
    static bool Read(const NJson::TJsonValue& jsonInfo, const std::initializer_list<TString>& fieldNames, TMaybe<T, TMaybePolicy>& result, const bool mustBe = false) {
        for (auto&& name : fieldNames) {
            if (mustBe && Read(jsonInfo, name, result, mustBe)) {
                return true;
            }
            if (!mustBe && !Read(jsonInfo, name, result, mustBe)) {
                return false;
            }
        }
        return !mustBe;

    }

    template <class T, class TMaybePolicy>
    static bool Read(const NJson::TJsonValue& jsonInfo, const TString& fieldName, TMaybe<T, TMaybePolicy>& result, const bool mustBe = false) {
        if (jsonInfo.Has(fieldName)) {
            T resultLocal;
            if (!Read(jsonInfo, fieldName, resultLocal, mustBe)) {
                return false;
            }
            result = resultLocal;
            return true;
        }
        return !mustBe;
    }
    Y_WARN_UNUSED_RESULT static bool Read(const NJson::TJsonValue& jsonInfo, const TString& fieldName, TInstant& result, const bool mustBe = false);

    template <class T>
    static void Write(NJson::TJsonValue& target, const TString& fieldName, const TMap<TString, T>& mapContainer) {
        NJson::TJsonValue& jsonRemappingInfo = target.InsertValue(fieldName, NJson::JSON_ARRAY);
        for (auto&& i : mapContainer) {
            NJson::TJsonValue& jsonRemapInfo = jsonRemappingInfo.AppendValue(NJson::JSON_MAP);
            jsonRemapInfo.InsertValue("from", i.first);
            jsonRemapInfo.InsertValue("to", i.second);
        }
    }

    template <class T>
    static void Write(NJson::TJsonValue& target, const TString& fieldName, const T& value, const TMaybe<T> defValue = {}) {
        CHECK_WITH_LOG(target.IsMap() || !target.IsDefined());
        Y_ASSERT(!target.Has(fieldName));
        if (defValue && *defValue == value) {
            return;
        }
        target.InsertValue(fieldName, value);
    }

    template <class T>
    static void WriteObject(NJson::TJsonValue& target, const TString& fieldName, const T& value) {
        CHECK_WITH_LOG(target.IsMap() || !target.IsDefined());
        Y_ASSERT(!target.Has(fieldName));
        target.InsertValue(fieldName, value.SerializeToJson());
    }

    static void Write(NJson::TJsonValue& target, const TString& fieldName, const TDuration& value, const TMaybe<TDuration> defValue = {}) {
        CHECK_WITH_LOG(target.IsMap() || !target.IsDefined());
        Y_ASSERT(!target.Has(fieldName));
        if (defValue && *defValue == value) {
            return;
        }
        target.InsertValue(fieldName, value.Seconds());
    }

    template <class TMaybePolicy>
    static void Write(NJson::TJsonValue& target, const TString& fieldName, const TMaybe<TInstant, TMaybePolicy>& value, const TMaybe<TInstant> defValue = {}) {
        WriteInstant(target, fieldName, value, defValue);
    }

    static void Write(NJson::TJsonValue& target, const TString& fieldName, const TInstant& value, const TMaybe<TInstant> defValue = {}) {
        WriteInstant(target, fieldName, value, defValue);
    }

    template <class T>
    static void WriteSerializable(NJson::TJsonValue& target, const TString& fieldName, const T& object) {
        CHECK_WITH_LOG(target.IsMap() || !target.IsDefined());
        Y_ASSERT(!target.Has(fieldName));
        target.InsertValue(fieldName, object.SerializeToJson());
    }

    template <class T, class TMaybePolicy>
    static void Write(NJson::TJsonValue& target, const TString& fieldName, const TMaybe<T, TMaybePolicy>& value, const TMaybe<T> defValue = {}) {
        if (!value) {
            return;
        }
        Write(target, fieldName, *value, defValue);
    }

    template<class T, class TMaybePolicy>
    static void WriteAsStringNullable(NJson::TJsonValue& target, const TString& fieldName, const TMaybe<T, TMaybePolicy>& value) {
        if (!value) {
            target.InsertValue(fieldName, NJson::JSON_NULL);
            return;
        }
        target.InsertValue(fieldName, ToString(*value));
    }

    template <class T, class TMaybePolicy>
    static void WriteObject(NJson::TJsonValue& target, const TString& fieldName, const TMaybe<T, TMaybePolicy>& value) {
        if (!value) {
            return;
        }
        WriteObject(target, fieldName, *value);
    }

    template <class TContainer>
    static void WriteContainerString(NJson::TJsonValue& target, const TString& fieldName, const TContainer& container, const bool writeEmpty = true) {
        if (!writeEmpty && container.empty()) {
            return;
        }
        CHECK_WITH_LOG(target.IsMap() || !target.IsDefined());
        Y_ASSERT(!target.Has(fieldName));
        target.InsertValue(fieldName, JoinSeq(",", container));
    }

    template <class TValue>
    static void WriteContainerStringMaybe(NJson::TJsonValue& target, const TString& fieldName, const TVector<TMaybe<TValue>>& data, const TString& emptyObj = "-") {
        CHECK_WITH_LOG(target.IsMap() || !target.IsDefined());
        Y_ASSERT(!target.Has(fieldName));
        TVector<TString> values;
        for (auto&& i : data) {
            if (i) {
                values.emplace_back(::ToString(*i));
            } else {
                values.emplace_back(emptyObj);
            }
        }
        target.InsertValue(fieldName, JoinSeq(",", values));
    }

    template <class TContainer>
    static void WriteContainerArray(NJson::TJsonValue& target, const TString& fieldName, const TContainer& container, const bool writeEmpty = true) {
        if (!writeEmpty && container.empty()) {
            return;
        }
        CHECK_WITH_LOG(target.IsMap() || !target.IsDefined());
        Y_ASSERT(!target.Has(fieldName));
        auto& jsonArray = target.InsertValue(fieldName, NJson::JSON_ARRAY);
        for (auto&& i : container) {
            jsonArray.AppendValue(i);
        }
    }

    template <class TContainer>
    static void WriteContainerArray(NJson::TJsonWriter& target, const TString& fieldName, const TContainer& container, const bool writeEmpty = true) {
        if (!writeEmpty && container.empty()) {
            return;
        }
        target.OpenArray(fieldName);
        for (auto&& i : container) {
            target.Write(i);
        }
        target.CloseArray();
    }


    template <class TContainer>
    static void WriteObjectsArray(NJson::TJsonValue& target, const TContainer& container) {
        CHECK_WITH_LOG(target.IsArray() || !target.IsDefined());
        target.SetType(NJson::JSON_ARRAY);
        for (auto&& object : container) {
            target.AppendValue(object.SerializeToJson());
        }
    }

    template <class TContainer>
    static void WriteObjectsArray(NJson::TJsonValue& target, const TString& fieldName, const TContainer& container, const bool writeEmpty = true) {
        if (!writeEmpty && container.empty()) {
            return;
        }
        CHECK_WITH_LOG(target.IsMap() || !target.IsDefined());
        NJson::TJsonValue& array = target.InsertValue(fieldName, NJson::JSON_ARRAY);
        WriteObjectsArray(array, container);
    }

    template <class TContainer>
    static void WriteContainerArrayStrings(NJson::TJsonValue& target, const TString& fieldName, const TContainer& container, const bool writeEmpty = true) {
        if (!writeEmpty && container.empty()) {
            return;
        }
        CHECK_WITH_LOG(target.IsMap() || !target.IsDefined());
        Y_ASSERT(!target.Has(fieldName));
        auto& jsonArray = target.InsertValue(fieldName, NJson::JSON_ARRAY);
        for (auto&& i : container) {
            jsonArray.AppendValue(::ToString(i));
        }
    }

    template <class TContainer>
    Y_WARN_UNUSED_RESULT static bool ReadContainer(const NJson::TJsonValue& jsonInfo, const TString& fieldName, TContainer& resultExt, const bool mustBe = false, const bool skipErrors = false, const bool mayBeEmpty = true, const char* splitSet = ", ") {
        auto gLogging = TFLRecords::StartContext()("field", fieldName);
        const NJson::TJsonValue* jsonValue;
        if (!jsonInfo.GetValuePointer(fieldName, &jsonValue)) {
            if (mustBe) {
                TFLEventLog::Error("no_field");
            }
            return !mustBe;
        }
        TContainer resultLocal;
        if (jsonValue->IsString()) {
            try {
                StringSplitter(jsonValue->GetString()).SplitBySet(splitSet).SkipEmpty().ParseInto(&resultLocal);
            } catch (...) {
                TFLEventLog::Error("cannot_parse_container_elements")("value", jsonValue->GetString());
                return false;
            }
        } else if (jsonValue->IsArray()) {
            using TValue = typename TContainer::value_type;
            auto insertOperator = std::inserter(resultLocal, resultLocal.begin());
            for (auto&& i : jsonValue->GetArraySafe()) {
                TValue value;
                if (!TryFromString<TValue>(i.GetStringRobust(), value)) {
                    if (skipErrors) {
                        continue;
                    }
                    TFLEventLog::Error("cannot_parse_container_element")("value", jsonValue->GetStringRobust());
                    return false;
                } else {
                    *insertOperator++ = std::move(value);
                }
            }
        } else {
            TFLEventLog::Error("incorrect_json_type")("field", fieldName)("type", jsonValue->GetType());
            return false;
        }
        if (resultLocal.empty() && !mayBeEmpty) {
            TFLEventLog::Error("empty_container_parsed");
            return false;
        }
        std::swap(resultLocal, resultExt);
        return true;
    }

    template <class TContainer, class TMaybePolicy>
    Y_WARN_UNUSED_RESULT static bool ReadContainer(const NJson::TJsonValue& jsonInfo, const TString& fieldName, TMaybe<TContainer, TMaybePolicy>& resultExt, const bool skipError = false, const bool mayBeEmpty = true, const char* splitSet = ", ") {
        if (!jsonInfo.Has(fieldName)) {
            return true;
        }
        TContainer cLocal;
        if (!ReadContainer(jsonInfo, fieldName, cLocal, true, skipError, mayBeEmpty, splitSet)) {
            TFLEventLog::Error("cannot parse container");
            return false;
        }
        resultExt = std::move(cLocal);
        return true;
    }

    template <class TContainer, class... TArgs>
    Y_WARN_UNUSED_RESULT static bool ReadObjectsContainerVariadic(const NJson::TJsonValue& jsonInfo, TContainer& result, const bool mustBe, const bool skipErrors, const bool mayBeEmpty, TArgs... args) {
        const NJson::TJsonValue::TArray* arrValue;
        if (!jsonInfo.GetArrayPointer(&arrValue)) {
            if (mustBe) {
                TFLEventLog::Error("field_value_is_not_array")("value", jsonInfo.GetStringRobust());
            }
            return !mustBe;
        }
        result.clear();
        using TValue = typename TContainer::value_type;
        auto insertOperator = std::inserter(result, result.end());
        for (auto&& i : *arrValue) {
            TValue value;
            if (!value.DeserializeFromJson(i, args...)) {
                if (skipErrors) {
                    continue;
                }
                TFLEventLog::Error("cannot_parse_container")("array", jsonInfo.GetStringRobust())("incorrect_value", i.GetStringRobust());
                return false;
            } else {
                *insertOperator++ = std::move(value);
            }
        }
        if (!mayBeEmpty && result.empty()) {
            TFLEventLog::Error("empty container");
            return false;
        }
        return true;
    }

    template <class TContainer>
    Y_WARN_UNUSED_RESULT static bool ReadObjectsContainer(const NJson::TJsonValue& jsonInfo, TContainer& result, const bool mustBe = false, const bool skipErrors = false, const bool mayBeEmpty = true) {
        return ReadObjectsContainerVariadic(jsonInfo, result, mustBe, skipErrors, mayBeEmpty);
    }

    template <class TContainer>
    Y_WARN_UNUSED_RESULT static bool ReadObjectsContainer(const NJson::TJsonValue& jsonInfo, const TString& fieldName, TContainer& result, const bool mustBe = false, const bool skipErrors = false, const bool mayBeEmpty = true) {
        auto gLogging = TFLRecords::StartContext()("field_name", fieldName);
        return ReadObjectsContainer(jsonInfo[fieldName], result, mustBe, skipErrors, mayBeEmpty);
    }

    template <class TContainer, class... TArgs>
    Y_WARN_UNUSED_RESULT static bool ReadObjectsContainerVariadic(const NJson::TJsonValue& jsonInfo, const TString& fieldName, TContainer& result, const bool mustBe, const bool skipErrors, const bool mayBeEmpty, TArgs... args) {
        auto gLogging = TFLRecords::StartContext()("field_name", fieldName);
        return ReadObjectsContainerVariadic(jsonInfo[fieldName], result, mustBe, skipErrors, mayBeEmpty, args...);
    }

    template <class TContainer>
    static void WriteObjectsContainer(NJson::TJsonValue& jsonInfo, const TContainer& container) {
        jsonInfo.SetType(NJson::JSON_ARRAY);
        for (auto&& i : container) {
            jsonInfo.AppendValue(i.SerializeToJson());
        }
    }

    template <class TContainer, class... TArgs>
    static void WriteObjectsContainerVariadicImpl(NJson::TJsonValue& jsonInfo, const TContainer& container, TArgs... args) {
        jsonInfo.SetType(NJson::JSON_ARRAY);
        for (auto&& i : container) {
            jsonInfo.AppendValue(i.SerializeToJson(args...));
        }
    }

    template <class TContainer, class... TArgs>
    static void WriteObjectsContainerVariadic(NJson::TJsonValue& jsonInfo, const TString& fieldName, const TContainer& container, TArgs... args) {
        auto& jsonObjects = jsonInfo.InsertValue(fieldName, NJson::JSON_ARRAY);
        WriteObjectsContainerVariadicImpl(jsonObjects, container, args...);
    }

    template <class TContainer>
    static void WriteObjectsContainer(NJson::TJsonValue& jsonInfo, const TString& fieldName, const TContainer& container) {
        auto& jsonObjects = jsonInfo.InsertValue(fieldName, NJson::JSON_ARRAY);
        WriteObjectsContainer(jsonObjects, container);
    }

    template <class TObject>
    Y_WARN_UNUSED_RESULT static bool ReadObject(const NJson::TJsonValue& jsonInfo, const TString& fieldName, TObject& result, const bool mustBe = false) {
        const NJson::TJsonValue* jsonValue;
        if (!jsonInfo.GetValuePointer(fieldName, &jsonValue)) {
            if (mustBe) {
                TFLEventLog::Error("no_field")("field", fieldName);
            }
            return !mustBe;
        }
        TObject value;
        if (!value.DeserializeFromJson(*jsonValue)) {
            TFLEventLog::Error("cannot_parse_object")("field", fieldName)("value", jsonValue->GetStringRobust());
            return false;
        }
        result = std::move(value);
        return true;
    }

    template <class TObject, class TMaybePolicy>
    Y_WARN_UNUSED_RESULT static bool ReadObject(const NJson::TJsonValue& jsonInfo, const TString& fieldName, TMaybe<TObject, TMaybePolicy>& result, const bool mustBe = false) {
        const NJson::TJsonValue* jsonValue;
        if (!jsonInfo.GetValuePointer(fieldName, &jsonValue)) {
            if (mustBe) {
                TFLEventLog::Error("no_field")("field", fieldName);
            }
            return !mustBe;
        }
        TObject value;
        if (!value.DeserializeFromJson(*jsonValue)) {
            TFLEventLog::Error("cannot_parse_object")("field", fieldName)("value", jsonValue->GetStringRobust());
            return false;
        }
        result = std::move(value);
        return true;
    }

    template <class TValue>
    Y_WARN_UNUSED_RESULT static bool ReadContainerMaybe(const NJson::TJsonValue& jsonInfo, const TString& fieldName, TVector<TMaybe<TValue>>& resultExt, const bool mustBe = false, const TString& emptyObj = "-", const char* splitSet = ", ") {
        const NJson::TJsonValue* jsonValue;
        if (!jsonInfo.GetValuePointer(fieldName, &jsonValue)) {
            if (mustBe) {
                TFLEventLog::Error("no_field")("field", fieldName);
            }
            return !mustBe;
        }
        if (jsonValue->IsString()) {
            try {
                TVector<TMaybe<TValue>> resultLocal;
                TVector<TString> values;
                StringSplitter(jsonValue->GetString()).SplitBySet(splitSet).SkipEmpty().Collect(&values);
                TValue val;
                for (auto&& i : values) {
                    if (i == emptyObj) {
                        resultLocal.emplace_back(TMaybe<TValue>());
                    } else if (TryFromString(i, val)) {
                        resultLocal.emplace_back(val);
                    } else {
                        TFLEventLog::Error("cannot_parse_container")("field", fieldName)("array", jsonValue->GetStringRobust())("value", i);
                        return false;
                    }
                }
                std::swap(resultExt, resultLocal);
                return true;
            } catch (...) {
                TFLEventLog::Error("cannot_split_container")("field", fieldName)("array", jsonValue->GetString());
                return false;
            }
        }
        if (jsonValue->IsArray()) {
            TVector<TMaybe<TValue>> resultLocal;
            for (auto&& i : jsonValue->GetArraySafe()) {
                if (!i.IsDefined()) {
                    resultLocal.emplace_back(TMaybe<TValue>());
                } else {
                    TValue val;
                    if (!TryFromString<TValue>(i.GetStringRobust(), val)) {
                        TFLEventLog::Error("cannot_parse_container")("field", fieldName)("array", jsonValue->GetStringRobust())("value", i.GetStringRobust());
                        return false;
                    } else {
                        resultLocal.emplace_back(val);
                    }
                }
            }
            std::swap(resultExt, resultLocal);
            return true;
        }
        TFLEventLog::Error("incorrect_json_type")("field", fieldName)("type", jsonValue->GetType());
        return false;
    }

    Y_WARN_UNUSED_RESULT static bool ReadContainer(const NJson::TJsonValue& jsonInfo, const TString& fieldName, TSet<TCiString>& resultExt, const bool mustBe = false, const char* splitSet = ", ") {
        const NJson::TJsonValue* jsonValue;
        if (!jsonInfo.GetValuePointer(fieldName, &jsonValue)) {
            if (mustBe) {
                TFLEventLog::Error("no_field")("field", fieldName);
            }
            return !mustBe;
        }
        if (jsonValue->IsString()) {
            try {
                TSet<TCiString> resultLocal;
                TSet<TString> stringsNormal;
                StringSplitter(jsonValue->GetString()).SplitBySet(splitSet).SkipEmpty().ParseInto(&stringsNormal);
                for (auto&& i : stringsNormal) {
                    resultLocal.emplace(i);
                }
                std::swap(resultLocal, resultExt);
                return true;
            } catch (...) {
                TFLEventLog::Error("cannot_parse_container")("field", fieldName)("array", jsonValue->GetString());
                return false;
            }
        }
        if (jsonValue->IsArray()) {
            TSet<TCiString> resultLocal;
            for (auto&& i : jsonValue->GetArraySafe()) {
                resultLocal.emplace(i.GetStringRobust());
            }
            std::swap(resultLocal, resultExt);
            return true;
        }
        TFLEventLog::Error("incorrect_json_type")("field", fieldName)("type", jsonValue->GetType());
        return false;
    }
};

#define JWRITE(target, name, value) {CHECK_WITH_LOG((target).IsMap() || !(target).IsDefined()); Y_ASSERT(!(target).Has(name)); (target).InsertValue((name), (value));};
#define JWRITE_DURATION(target, name, value) TJsonProcessor::WriteDurationInt(target, name, value);
#define JWRITE_INSTANT(target, name, value) {CHECK_WITH_LOG(target.IsMap() || !target.IsDefined()); Y_ASSERT(!target.Has(name)); (target).InsertValue((name), (value.Seconds()));};
#define JWRITE_ENUM(target, name, value) {CHECK_WITH_LOG(target.IsMap() || !target.IsDefined()); Y_ASSERT(!target.Has(name)); (target).InsertValue((name), (::ToString(value)));};
#define JWRITE_ENUM_DEF(target, name, value, valueDef) {if (valueDef != value) {CHECK_WITH_LOG(target.IsMap() || !target.IsDefined()); Y_ASSERT(!target.Has(name)); (target).InsertValue((name), (::ToString(value)));}};
#define JWRITE_DEF(target, name, value, valueDef) \
if ((value) != (valueDef)) \
{\
    CHECK_WITH_LOG(target.IsMap() || !target.IsDefined());\
    Y_ASSERT(!target.Has(name)); \
    (target).InsertValue((name), (value));\
};
#define JWRITE_DEF_NULL(target, name, value, valueDef) \
{\
    CHECK_WITH_LOG(target.IsMap() || !target.IsDefined());\
    Y_ASSERT(!target.Has(name)); \
    if ((value) != (valueDef)) {\
        (target).InsertValue((name), (value)); \
    } else { \
        (target).InsertValue((name), (NJson::JSON_NULL)); \
    } \
};
#define JWRITE_DURATION_DEF(target, name, value, valueDef) TJsonProcessor::WriteDurationInt(target, name, value, valueDef);
#define JWRITE_INSTANT_DEF(target, name, value, valueDef) TJsonProcessor::WriteInstant(target, name, value, valueDef);

#define JWRITE_INSTANT_ISOFORMAT_DEF(target, name, value, valueDef) \
{\
    CHECK_WITH_LOG(target.IsMap() || !target.IsDefined());\
    Y_ASSERT(!target.Has(name)); \
    if ((value) != (valueDef)) {\
        (target).InsertValue((name), (value.ToString())); \
    }; \
};
#define JWRITE_INSTANT_ISOFORMAT_DEF_NULL(target, name, value, valueDef) \
{\
    CHECK_WITH_LOG(target.IsMap() || !target.IsDefined());\
    Y_ASSERT(!target.Has(name)); \
    if ((value) != (valueDef)) {\
        (target).InsertValue((name), (value.ToString())); \
    } else { \
        (target).InsertValue((name), (NJson::JSON_NULL)); \
    } \
};

#define JREAD_COMMON(source, name, value, type)\
{\
    if (!(source).Has(name) || !((source)[name].Is ## type())) {\
        return false;\
    } else {\
        value = (source)[name].Get ## type();\
    }\
}

#define JREAD_COMMON_OPT(source, name, value, type)\
{\
    if ((source).Has(name)) {\
        if (!((source)[name].Is ## type())) {\
            return false; \
        } else {\
            value = (source)[name].Get ## type(); \
        }\
    }\
}

#define JREAD_COMMON_NULLABLE_OPT(source, name, value, type)\
{\
    if ((source).Has(name) && !((source)[name].IsNull())) {\
        if (!((source)[name].Is ## type())) {\
            return false; \
        } else {\
            value = (source)[name].Get ## type(); \
        }\
    }\
}

#define JREAD_STRING(source, name, value) JREAD_COMMON(source, name, value, String);
#define JREAD_STRING_OPT(source, name, value) JREAD_COMMON_OPT(source, name, value, String);
#define JREAD_STRING_NULLABLE_OPT(source, name, value) JREAD_COMMON_NULLABLE_OPT(source, name, value, String);

#define JREAD_INT(source, name, value) JREAD_COMMON(source, name, value, Integer);
#define JREAD_INT_OPT(source, name, value) JREAD_COMMON_OPT(source, name, value, Integer);

#define JREAD_UINT(source, name, value) JREAD_COMMON(source, name, value, UInteger);
#define JREAD_UINT_OPT(source, name, value) JREAD_COMMON_OPT(source, name, value, UInteger);
#define JREAD_UINT_NULLABLE_OPT(source, name, value) JREAD_COMMON_NULLABLE_OPT(source, name, value, UInteger);

#define JREAD_DOUBLE(source, name, value) JREAD_COMMON(source, name, value, Double);
#define JREAD_DOUBLE_OPT(source, name, value) JREAD_COMMON_OPT(source, name, value, Double);
#define JREAD_DOUBLE_NULLABLE_OPT(source, name, value) JREAD_COMMON_NULLABLE_OPT(source, name, value, Double);

#define JREAD_BOOL(source, name, value) JREAD_COMMON(source, name, value, Boolean);
#define JREAD_BOOL_OPT(source, name, value) JREAD_COMMON_OPT(source, name, value, Boolean);

#define JREAD_DURATION(source, name, value)\
if (!TJsonProcessor::Read(source, name, value, true)) {\
    return false;\
}

#define JREAD_DURATION_OPT(source, name, value)\
if (!TJsonProcessor::Read(source, name, value, false)) {\
    return false;\
}

#define JREAD_CONTAINER(source, name, value)\
if (!TJsonProcessor::ReadContainer(source, name, value, true)) {\
    return false;\
}

#define JREAD_CONTAINER_OPT(source, name, value)\
if (!TJsonProcessor::ReadContainer(source, name, value, false)) {\
    return false;\
}

#define JREAD_INSTANT(source, name, value)\
if (!TJsonProcessor::Read(source, name, value, true)) {\
    return false;\
}

#define JREAD_INSTANT_OPT(source, name, value)\
if (!TJsonProcessor::Read(source, name, value, false)) {\
    return false;\
}

#define JREAD_INSTANT_ISOFORMAT_NULLABLE_OPT(source, name, value)\
{\
    if ((source).Has(name) && !((source)[name].IsNull()))  {\
        if (!((source)[name].IsString())) {\
            TFLEventLog::Error("incorrect_json_type")("field", name)("type", (source)[name].GetType());\
            return false;\
        } else {\
            if (!TInstant::TryParseIso8601((source)[name].GetString(), value)) {\
                TFLEventLog::Error("cannot_parse_object")("field", name)("value", (source)[name].GetStringRobust());\
                return false;\
            }\
        }\
    }\
}

#define JREAD_FROM_STRING_IMPL(source, name, value, method)\
{\
    if (!(source).Has(name) || !((source)[name].IsString())) {\
        TFLEventLog::Error("incorrect_json_type")("field", name)("type", (source)[name].GetType());\
        return false;\
    } else {\
        if (!method((source)[name].GetString(), value)) {\
            TFLEventLog::Error("cannot_parse_object")("field", name)("value", (source)[name].GetStringRobust());\
            return false;\
        }\
    }\
}

#define JREAD_FROM_STRING_OPT_IMPL(source, name, value, method)\
{\
    if ((source).Has(name)) {\
        if (!((source)[name].IsString())) {\
            TFLEventLog::Error("incorrect_json_type")("field", name)("type", (source)[name].GetType());\
            return false; \
        } else {\
            if (!method((source)[name].GetString(), value)) {\
                TFLEventLog::Error("cannot_parse_object")("field", name)("value", (source)[name].GetStringRobust());\
                return false; \
            }\
        }\
    }\
}

#define JREAD_FROM_STRING_OPT(source, name, value) JREAD_FROM_STRING_OPT_IMPL(source, name, value, TryFromString);
#define JREAD_FROM_STRING(source, name, value) JREAD_FROM_STRING_IMPL(source, name, value, TryFromString);
#define JWRITE_AS_STRING(source, name, value) JWRITE(source, name, ::ToString(value));
