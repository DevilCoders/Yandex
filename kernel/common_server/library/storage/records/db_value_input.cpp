#include "db_value_input.h"
#include <library/cpp/string_utils/base64/base64.h>
#include <util/string/hex.h>
#include <util/string/cast.h>

namespace NCS {
    namespace NStorage {

        TString TBinaryData::GetEncodedData() const {
            if (!EncodedDataCache) {
                if (Deprecated) {
                    EncodedDataCache = Base64Encode(OriginalData);
                } else {
                    EncodedDataCache = HexEncode(OriginalData);
                }
            }
            return *EncodedDataCache;
        }

        class TDBValueInputToJson {
        public:
            template <class T>
            NJson::TJsonValue operator()(const T& value) const {
                return value;
            }
            NJson::TJsonValue operator()(const TGUID& value) const {
                return value.AsUuidString();
            }
            NJson::TJsonValue operator()(const TBinaryData& value) const {
                return value.GetOriginalData();
            }
            NJson::TJsonValue operator()(const TNull& /*value*/) const {
                return NJson::JSON_NULL;
            }
        };

        NJson::TJsonValue TDBValueInputOperator::SerializeToJson(const TDBValueInput& value) {
            TDBValueInputToJson pred;
            return std::visit(pred, value);
        }

        class TDBValueInputToString {
        public:
            template <class T>
            TString operator()(const T& value) const {
                return ::ToString(value);
            }
            TString operator()(const TGUID& value) const {
                return value.AsUuidString();
            }
            TString operator()(const TBinaryData& value) const {
                return value.GetOriginalData();
            }
            TString operator()(const TNull& /*value*/) const {
                return "";
            }
        };

        TString TDBValueInputOperator::SerializeToString(const TDBValueInput& value) {
            TDBValueInputToString pred;
            return std::visit(pred, value);
        }

        NCS::NStorage::TDBValueInput TDBValueInputOperator::ParseFromJson(const NJson::TJsonValue& jsonInfo) {
            switch (jsonInfo.GetType()) {
                case NJson::EJsonValueType::JSON_ARRAY:
                case NJson::EJsonValueType::JSON_MAP:
                    return jsonInfo.GetStringRobust();
                case NJson::EJsonValueType::JSON_BOOLEAN:
                    return jsonInfo.GetBoolean();
                case NJson::EJsonValueType::JSON_DOUBLE:
                    return jsonInfo.GetDouble();
                case NJson::EJsonValueType::JSON_INTEGER:
                    return jsonInfo.GetInteger();
                case NJson::EJsonValueType::JSON_UINTEGER:
                    return jsonInfo.GetUInteger();
                case NJson::EJsonValueType::JSON_NULL:
                case NJson::EJsonValueType::JSON_UNDEFINED:
                    return TNull();
                case NJson::EJsonValueType::JSON_STRING:
                    return jsonInfo.GetString();
            }
        }

        class TDBValueInputToDBValueOutput {
        public:
            template <class T>
            TMaybe<TDBValue> operator()(const T& value) const {
                return value;
            }
            TMaybe<TDBValue> operator()(const ui32 value) const {
                return (ui64)value;
            }
            TMaybe<TDBValue> operator()(const i32 value) const {
                return (i64)value;
            }
            TMaybe<TDBValue> operator()(const TGUID& value) const {
                return value.AsUuidString();
            }
            TMaybe<TDBValue> operator()(const TBinaryData& value) const {
                return value.GetEncodedData();
            }
            TMaybe<TDBValue> operator()(const TNull& /*value*/) const {
                return Nothing();
            }
        };

        TMaybe<NCS::NStorage::TDBValue> TDBValueInputOperator::ToDBValue(const TDBValueInput& input) {
            TDBValueInputToDBValueOutput pred;
            return std::visit(pred, input);
        }

    }
}
