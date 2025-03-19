#include "db_value.h"
#include <kernel/common_server/library/logging/events.h>

namespace NCS {
    namespace NStorage{
        TMaybe<TDBValue> TDBValueOperator::ParseString(const TStringBuf sb, const EColumnType columnType) {
            double dValue;
            i64 i64Value;
            ui64 ui64Value;
            bool bValue;
            switch (columnType) {
                case EColumnType::Double:
                    if (!TryFromString<double>(sb, dValue)) {
                        TFLEventLog::Error("cannot cast value to field type")("expected_type", columnType)("value", sb);
                        return Nothing();
                    } else {
                        return dValue;
                    }
                case EColumnType::I32:
                case EColumnType::I64:
                    if (!TryFromString<i64>(sb, i64Value)) {
                        TFLEventLog::Error("cannot cast value to field type")("expected_type", columnType)("value", sb);
                        return Nothing();
                    } else {
                        return i64Value;
                    }
                case EColumnType::UI64:
                    if (!TryFromString<ui64>(sb, ui64Value)) {
                        TFLEventLog::Error("cannot cast value to field type")("expected_type", columnType)("value", sb);
                        return Nothing();
                    } else {
                        return ui64Value;
                    }
                case EColumnType::Boolean:
                    if (!TryFromString<bool>(sb, bValue)) {
                        TFLEventLog::Error("cannot cast value to field type")("expected_type", columnType)("value", sb);
                        return Nothing();
                    } else {
                        return bValue;
                    }
                case EColumnType::Text:
                case EColumnType::Binary:
                    return TString(sb.data(), sb.size());
            }
        }

        bool TDBValueOperator::TryGetString(const NStorage::TDBValue& value, TString& result) {
            auto val = std::get_if<TString>(&value);
            if (!val) {
                return false;
            }
            result = *val;
            return true;
        }

        TMaybe<EColumnType> TDBValueOperator::FromJsonType(const NJson::EJsonValueType type) {
            switch (type) {
                case NJson::JSON_BOOLEAN:
                    return EColumnType::Boolean;
                case NJson::JSON_INTEGER:
                    return EColumnType::I64;
                case NJson::JSON_DOUBLE:
                    return EColumnType::Double;
                case NJson::JSON_STRING:
                case NJson::JSON_MAP:
                case NJson::JSON_ARRAY:
                    return EColumnType::Text;
                case NJson::JSON_UINTEGER:
                    return EColumnType::UI64;
                case NJson::JSON_NULL:
                case NJson::JSON_UNDEFINED:
                    return Nothing();
            }
        }

        bool TDBValueOperator::DeserializeFromJson(NStorage::TDBValue& value, const NJson::TJsonValue& jsonValue) {
            switch (jsonValue.GetType()) {
                case NJson::JSON_BOOLEAN:
                    value = jsonValue.GetBoolean();
                    return true;
                case NJson::JSON_INTEGER:
                    value = jsonValue.GetInteger();
                    return true;
                case NJson::JSON_DOUBLE:
                    value = jsonValue.GetDouble();
                    return true;
                case NJson::JSON_STRING:
                    value = jsonValue.GetString();
                    return true;
                case NJson::JSON_MAP:
                case NJson::JSON_ARRAY:
                    value = jsonValue.GetStringRobust();
                    return true;
                case NJson::JSON_UINTEGER:
                    value = jsonValue.GetUInteger();
                    return true;
                case NJson::JSON_NULL:
                case NJson::JSON_UNDEFINED:
                    TFLEventLog::Error("cannot parse db_value from json")("json_type", jsonValue.GetType());
                    return false;
            }
        }

        NJson::TJsonValue TDBValueOperator::SerializeToJson(const NStorage::TDBValue& value) {
            const auto pred = [](const auto& v) ->NJson::TJsonValue {
                return v;
            };
            return std::visit<NJson::TJsonValue>(pred, value);
        }

        TString TDBValueOperator::SerializeToString(const NStorage::TDBValue& value) {
            const auto pred = [](const auto& v) ->TString {
                return ::ToString(v);
            };
            return std::visit<TString>(pred, value);
        }

    }
}
