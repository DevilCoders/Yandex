#pragma once
#include <kernel/common_server/util/accessor.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/json/writer/json_value.h>
#include <util/generic/array_ref.h>
#include <util/generic/fwd.h>
#include <kernel/common_server/library/storage/records/record.h>
#include <kernel/common_server/library/storage/records/t_record.h>
#include <library/cpp/logger/global/global.h>
#include <util/string/cast.h>
#include <kernel/common_server/library/logging/events.h>
#include <util/string/hex.h>
#include "abstract.h"

namespace NCS {
    namespace NStorage {
        class TBaseDecoder {
        private:
            static constexpr ui32 kRawDataSize = 64 * 1024 * 1.5;
            CS_ACCESS(TBaseDecoder, bool, Strict, true);
        public:

            void TakeColumnsInfo(const TVector<TOrderedColumn>& /*columns*/) {

            }

            TBaseDecoder() = default;
            explicit TBaseDecoder(const bool strict)
                : Strict(strict) {

            }

            template <class TProto>
            static TString SerializeProtoToBase64String(const TProto& proto) {
                return Base64Encode(proto.SerializeAsString());
            }

            template <class TProto>
            Y_WARN_UNUSED_RESULT static bool DeserializeProtoFromBase64String(const TString& data, TProto& proto) {
                try {
                    const TString protoString = Base64Decode(data);
                    TProto protoLocal;
                    if (!protoLocal.ParsePartialFromArray(protoString.data(), protoString.Size())) {
                        TFLEventLog::Error("cannot parse partial protobuf for DeserializeProtoFromBase64String")("raw_data", data, kRawDataSize);
                        return false;
                    }
                    std::swap(protoLocal, proto);
                    return true;
                } catch (...) {
                    TFLEventLog::Error(CurrentExceptionMessage());
                    return false;
                }
            }

            template <class TObject>
            Y_WARN_UNUSED_RESULT static bool DeserializeObjectsFromJsonImpl(const NJson::TJsonValue& jsonData, const TString& objectsPath, TVector<TObject>& result) {
                const NJson::TJsonValue::TArray* jsonArray = nullptr;
                if (!jsonData[objectsPath].GetArrayPointer(&jsonArray)) {
                    TFLEventLog::Log("no " + objectsPath + " json node");
                    return false;
                }
                for (auto&& i : *jsonArray) {
                    TObject object;
                    if (!TBaseDecoder::DeserializeFromJson(object, i)) {
                        TFLEventLog::Log("cannot parse object from " + objectsPath);
                        return false;
                    }
                    result.emplace_back(std::move(object));
                }
                return true;
            }

            template <class TObject>
            static void SerializeObjectsToJson(NJson::TJsonValue& jsonData, const TString& objectsPath, const TVector<TObject>& objects) {
                NJson::TJsonValue& jsonObjects = jsonData.InsertValue(objectsPath, NJson::JSON_ARRAY);
                for (auto&& i : objects) {
                    jsonObjects.AppendValue(i.SerializeToTableRecord().BuildWT().SerializeToJson());
                }
            }

            static TString GetFieldsForRequest() {
                return "*";
            }

            static bool NeedVerboseParsingErrorLogging() {
                return true;
            }

            template <class T>
            Y_WARN_UNUSED_RESULT static bool DeserializeFromJson(T& result, const NJson::TJsonValue& jsonInfo) {
                TVector<TOrderedColumn> columns;
                TVector<TString> values;
                TVector<TStringBuf> valuesBuf;
                if (!GetRemap(jsonInfo, values, valuesBuf, columns)) {
                    return false;
                }
                typename T::TDecoder decoder(TOrderedColumn::BuildRemap(columns));
                decoder.TakeColumnsInfo(columns);
                decoder.SetStrict(false);
                try {
                    return result.DeserializeWithDecoder(decoder, valuesBuf);
                } catch (...) {
                    TFLEventLog::Error(CurrentExceptionMessage());
                    return false;
                }
            }

            template <class T>
            Y_WARN_UNUSED_RESULT static bool DeserializeFromTableRecordCommon(T& result, const NStorage::TTableRecordWT& record, const bool strict) {
                TVector<TString> valuesOutput;
                valuesOutput.reserve(record.size());
                TVector<TOrderedColumn> columns;
                for (auto&& [key, value] : record) {
                    valuesOutput.emplace_back(TDBValueOperator::SerializeToString(value));
                    columns.emplace_back(key, EColumnType::Text);
                }
                TVector<TStringBuf> valuesBuf;
                for (auto&& i : valuesOutput) {
                    valuesBuf.emplace_back(i);
                }
                typename T::TDecoder decoder(TOrderedColumn::BuildRemap(columns));
                decoder.TakeColumnsInfo(columns);
                decoder.SetStrict(strict);
                try {
                    return result.DeserializeWithDecoder(decoder, valuesBuf);
                } catch (...) {
                    TFLEventLog::Error(CurrentExceptionMessage());
                    return false;
                }
            }

            template <class T>
            Y_WARN_UNUSED_RESULT static bool DeserializeFromTableRecordStrictable(T& result, const NStorage::TTableRecordWT& record, const bool strict) {
                return DeserializeFromTableRecordCommon<T>(result, record, strict);
            }

            const TStringBuf& GetStringValue(const i32 idx, const TConstArrayRef<TStringBuf>& values) const {
                if (idx >= 0) {
                    CHECK_WITH_LOG(values.size() > (size_t)idx);
                    return values[idx];
                } else {
                    CHECK_WITH_LOG(!Strict);
                    return Default<TStringBuf>();
                }
            }

            bool GetJsonValue(const i32 idx, const TConstArrayRef<TStringBuf>& values, NJson::TJsonValue& jsonResult, const bool mayBeEmpty = true) const;

            template <class TProto>
            bool GetProtoValueDeprecated(const i32 idx, const TConstArrayRef<TStringBuf>& values, TProto& proto) const {
                try {
                    if (idx < 0) {
                        return false;
                    }
                    CHECK_WITH_LOG(values.size() > (size_t)idx);
                    const TString protoStr = Base64Decode(values[idx]);
                    if (!proto.ParsePartialFromArray(protoStr.data(), protoStr.size())) {
                        TFLEventLog::Error("cannot parse protobuf in GetProtoValueDeprecated")("raw_data", values[idx], kRawDataSize);
                        return false;
                    }
                    return true;
                } catch (...) {
                    TFLEventLog::Error("exception on protobuf parsing in GetProtoValueDeprecated")("message", CurrentExceptionMessage());
                    return false;
                }
            }

            bool GetValueBytes(const i32 idx, const TConstArrayRef<TStringBuf>& values, TString& value) const noexcept;
            bool GetValueBytesPacked(const EDataCodec codec, const i32 idx, const TConstArrayRef<TStringBuf>& values, TString& value) const noexcept;
            bool GetValueBytesPackedAuto(const i32 idx, const TConstArrayRef<TStringBuf>& values, TString& value) const noexcept;

            template <class TProto>
            bool GetProtoValueBytes(const i32 idx, const TConstArrayRef<TStringBuf>& values, TProto& proto) const noexcept {
                TString protoStr;
                if (!GetValueBytes(idx, values, protoStr)) {
                    TFLEventLog::Error("cannot take bytes from values on db parsing");
                    return false;
                }
                try {
                    if (!proto.ParsePartialFromArray(protoStr.data(), protoStr.size())) {
                        TFLEventLog::Error("cannot parse protobuf in GetProtoValueBytes")("raw_data", Base64Encode(protoStr), kRawDataSize);
                        return false;
                    }
                    return true;
                } catch (...) {
                    TFLEventLog::Error("exception on protobuf parsing in GetProtoValueBytes")("message", CurrentExceptionMessage());
                    return false;
                }
            }

            template <class TProto>
            bool GetProtoValueBytesPacked(const EDataCodec codec, const i32 idx, const TConstArrayRef<TStringBuf>& values, TProto& proto) const {
                TString protoStr;
                if (!GetValueBytesPacked(codec, idx, values, protoStr)) {
                    return false;
                }
                try {
                    if (!proto.ParsePartialFromArray(protoStr.data(), protoStr.size())) {
                        TFLEventLog::Error("cannot parse partial protobuf for GetProtoValueBytesPacked")("raw_data", protoStr, kRawDataSize);
                        return false;
                    }
                    return true;
                } catch (...) {
                    TFLEventLog::Error(CurrentExceptionMessage());
                    return false;
                }
            }

            template <class TProto>
            bool GetProtoValueBytesPackedAuto(const i32 idx, const TConstArrayRef<TStringBuf>& values, TProto& proto) const {
                TString protoStr;
                if (!GetValueBytesPackedAuto(idx, values, protoStr)) {
                    return false;
                }
                try {
                    if (!proto.ParsePartialFromArray(protoStr.data(), protoStr.size())) {
                        TFLEventLog::Error("cannot parse partial protobuf for GetProtoValueBytesPackedAuto")("raw_data", protoStr, kRawDataSize);
                        return false;
                    }
                    return true;
                } catch (...) {
                    TFLEventLog::Error(CurrentExceptionMessage());
                    return false;
                }
            }

            template <class TProto>
            bool GetProtoValueOptional(const i32 idx, const TConstArrayRef<TStringBuf>& values, TProto& proto) const {
                try {
                    if (idx >= 0) {
                        CHECK_WITH_LOG(values.size() > (size_t)idx);
                        if (!values[idx]) {
                            return true;
                        }
                        const TString protoStr = Base64Decode(values[idx]);
                        if (!proto.ParsePartialFromArray(protoStr.data(), protoStr.size())) {
                            TFLEventLog::Error("cannot parse partial protobuf for GetProtoValueOptional")("raw_data", protoStr, kRawDataSize);
                            return false;
                        }
                        return true;
                    } else {
                        return false;
                    }
                } catch (...) {
                    TFLEventLog::Error(CurrentExceptionMessage());
                    return false;
                }
            }

            template <class TProto>
            bool GetProtoValueBytesOptional(const i32 idx, const TConstArrayRef<TStringBuf>& values, TProto& proto) const {
                try {
                    if (idx >= 0) {
                        CHECK_WITH_LOG(values.size() > (size_t)idx);
                        if (!values[idx]) {
                            return true;
                        }
                        const TStringBuf protoStr = values[idx];
                        if (!proto.ParsePartialFromArray(protoStr.data(), protoStr.size())) {
                            TFLEventLog::Error("cannot parse partial protobuf for GetProtoValueBytesOptional")("raw_data", protoStr, kRawDataSize);
                            return false;
                        }
                        return true;
                    } else {
                        return false;
                    }
                } catch (...) {
                    TFLEventLog::Error(CurrentExceptionMessage());
                    return false;
                }
            }

            template <class T>
            bool GetValueAs(const i32 idx, const TConstArrayRef<TStringBuf>& values, T& result) const {
                if (idx >= 0) {
                    CHECK_WITH_LOG(values.size() > (size_t)idx);
                    return TryFromString(values[idx], result);
                } else {
                    return false;
                }
            }

            template <class T>
            bool GetValueMaybeAs(const i32 idx, const TConstArrayRef<TStringBuf>& values, T& result) const {
                if (idx >= 0) {
                    CHECK_WITH_LOG(values.size() > (size_t)idx);
                    if (!!values[idx]) {
                        typename T::value_type localVariable;
                        if (!TryFromString(values[idx], localVariable)) {
                            return false;
                        }
                        result = localVariable;
                    }
                    return true;
                } else {
                    return true;
                }
            }

            template <>
            bool GetValueAs<TString>(const i32 idx, const TConstArrayRef<TStringBuf>& values, TString& result) const {
                if (idx >= 0) {
                    CHECK_WITH_LOG(values.size() > (size_t)idx);
                    result = TString(values[idx]);
                    return true;
                } else {
                    return false;
                }
            }

            i32 GetFieldDecodeIndex(const TString& fieldName, const TMap<TString, ui32>& decoderBase) const {
                auto it = decoderBase.find(fieldName);
                if (it != decoderBase.end()) {
                    return it->second;
                } else {
                    //            CHECK_WITH_LOG(!Strict) << fieldName << Endl;
                    return -1;
                }
            }

            i32 GetFieldDecodeIndexOptional(const TString& fieldName, const TMap<TString, ui32>& decoderBase) const {
                auto it = decoderBase.find(fieldName);
                if (it != decoderBase.end()) {
                    return it->second;
                } else {
                    return -1;
                }
            }

        private:
            static bool GetRemap(const NJson::TJsonValue& jsonInfo, TVector<TString>& values, TVector<TStringBuf>& valuesBuf, TVector<TOrderedColumn>& remap);
        };

        class TSimpleDecoder: public TBaseDecoder {
        private:
            using TFieldsRemapper = TMap<TString, ui32>;
            CSA_READONLY_DEF(TVector<TOrderedColumn>, Columns);
            CSA_READONLY_DEF(TFieldsRemapper, Remapper);
        public:
            TSimpleDecoder() = default;

            void TakeColumnsInfo(const TVector<TOrderedColumn>& columns) {
                Columns = columns;
            }

            TSimpleDecoder(const TFieldsRemapper& decoderBase)
                : Remapper(decoderBase)
            {
            }
        };
    }
}

#define DECODER_FIELD(fieldName) CS_ACCESS(TDecoder, i32, fieldName, -1);

#define READ_DECODER_VALUE(decoder, values, fieldName) {\
    if (decoder.Get ## fieldName() >= 0) {\
        CHECK_WITH_LOG(values.size() > (size_t)decoder.Get ## fieldName()); \
        if (!TryFromString(values[decoder.Get ## fieldName()], fieldName)) {\
            TFLEventLog::Error("Cannot parse field")("field_name", #fieldName);\
            return false;\
        }\
    } else {\
        CHECK_WITH_LOG(!decoder.GetStrict()) << #fieldName << Endl;\
    }\
}

#define READ_DECODER_VALUE_OPT(decoder, values, fieldName) {\
    if (decoder.Get ## fieldName() >= 0) {\
        CHECK_WITH_LOG(values.size() > (size_t)decoder.Get ## fieldName()); \
        if (!!values[decoder.Get ## fieldName()]) {\
            if (!TryFromString(values[decoder.Get ## fieldName()], fieldName)) {\
                TFLEventLog::Error("Cannot parse field")("field_name", #fieldName);\
                return false;\
            }\
        }\
    } else {\
        CHECK_WITH_LOG(!decoder.GetStrict()) << #fieldName << Endl;\
    }\
}

#define READ_DECODER_VALUE_DEF(decoder, values, fieldName, defValue) {\
    if (decoder.Get ## fieldName() >= 0) {\
        CHECK_WITH_LOG(values.size() > (size_t)decoder.Get ## fieldName());\
        if (!TryFromString(values[decoder.Get ## fieldName()], fieldName)) {\
            fieldName = defValue;\
        }\
    } else {\
        CHECK_WITH_LOG(!decoder.GetStrict()) << #fieldName << Endl;\
    }\
}

#define READ_DECODER_VALUE_TEMP(decoder, values, variable, fieldName) {\
    if (decoder.Get ## fieldName() >= 0) {\
        CHECK_WITH_LOG(values.size() > (size_t)decoder.Get ## fieldName());\
        if (!TryFromString(values[decoder.Get ## fieldName()], variable)) {\
            TFLEventLog::Error("Cannot parse field")("field_name", #fieldName);\
            return false;\
        }\
    } else {\
        CHECK_WITH_LOG(!decoder.GetStrict()) << #fieldName << Endl;\
    }\
}

#define READ_DECODER_VALUE_TEMP_OPT(decoder, values, variable, fieldName) {\
    if (decoder.Get ## fieldName() >= 0) {\
        CHECK_WITH_LOG(values.size() > (size_t)decoder.Get ## fieldName());\
        TryFromString(values[decoder.Get ## fieldName()], variable);\
    } else {\
        CHECK_WITH_LOG(!decoder.GetStrict()) << #fieldName << Endl;\
    }\
}

#define READ_DECODER_VALUE_INSTANT(decoder, values, fieldName) {\
    ui64 ts;\
    if (decoder.Get ## fieldName() >= 0) {\
        CHECK_WITH_LOG(values.size() > (size_t)decoder.Get ## fieldName());\
        if (!TryFromString(values[decoder.Get ## fieldName()], ts)) {\
            TFLEventLog::Error("Cannot parse field")("field_name", #fieldName);\
            return false; \
        }\
        fieldName = TInstant::Seconds(ts); \
    } else {\
        CHECK_WITH_LOG(!decoder.GetStrict()) << #fieldName << Endl;\
    }\
}

#define READ_DECODER_VALUE_INSTANT_OPT(decoder, values, fieldName) {\
    ui64 ts;\
    if (decoder.Get ## fieldName() >= 0) {\
        CHECK_WITH_LOG(values.size() > (size_t)decoder.Get ## fieldName());\
        if (!!values[decoder.Get ## fieldName()]) {\
            if (!TryFromString(values[decoder.Get ## fieldName()], ts)) {\
                TFLEventLog::Error("Cannot parse field")("field_name", #fieldName);\
                return false; \
            }\
            fieldName = TInstant::Seconds(ts);\
        }\
    } else {\
        CHECK_WITH_LOG(!decoder.GetStrict()) << #fieldName << Endl;\
    }\
}

#define READ_DECODER_VALUE_DURATION(decoder, values, fieldName) {\
    if (decoder.Get ## fieldName() >= 0) {\
        CHECK_WITH_LOG(values.size() > (size_t)decoder.Get ## fieldName());\
        ui64 ts;\
        if (!TryFromString(values[decoder.Get ## fieldName()], ts)) {\
            TFLEventLog::Error("Cannot parse field")("field_name", #fieldName);\
            return false; \
        }\
        fieldName = TDuration::Seconds(ts);\
    } else {\
        CHECK_WITH_LOG(!decoder.GetStrict()) << #fieldName << Endl;\
    }\
}

#define READ_DECODER_VALUE_DURATION_OPT(decoder, values, fieldName) {\
    if (decoder.Get ## fieldName() >= 0) {\
        CHECK_WITH_LOG(values.size() > (size_t)decoder.Get ## fieldName());\
        ui64 ts;\
        TDuration dts;\
        if (TryFromString(values[decoder.Get ## fieldName()], ts)) {\
            fieldName = TDuration::Seconds(ts);\
        } else if (TryFromString(values[decoder.Get ## fieldName()], dts)) {\
            fieldName = dts;\
        }\
    } else {\
        CHECK_WITH_LOG(!decoder.GetStrict()) << #fieldName << Endl;\
    }\
}

#define READ_DECODER_VALUE_JSON(decoder, values, jsonValue, fieldName) {\
    if (decoder.Get ## fieldName() >= 0) {\
        CHECK_WITH_LOG(values.size() > (size_t)decoder.Get ## fieldName());\
        if (!NJson::ReadJsonFastTree(values[decoder.Get ## fieldName()], &jsonValue)) {\
            TFLEventLog::Error("Cannot parse field")("field_name", #fieldName);\
            return false; \
        }\
    } else {\
        CHECK_WITH_LOG(!decoder.GetStrict()) << #fieldName << Endl;\
    }\
}

#define READ_DECODER_VALUE_INSTANT_ISOFORMAT_OPT(decoder, values, fieldName) {\
    if (decoder.Get ## fieldName() >= 0) {\
        CHECK_WITH_LOG(values.size() > (size_t)decoder.Get ## fieldName());\
        TInstant::TryParseIso8601(values[decoder.Get ## fieldName()], fieldName);\
    } else {\
        CHECK_WITH_LOG(!decoder.GetStrict()) << #fieldName << Endl;\
    }\
}

#define READ_DECODER_VALUE_STRING_ARRAY(decoder, values, fieldName) {\
    if (decoder.Get ## fieldName() >= 0) {\
        CHECK_WITH_LOG(values.size() > (size_t)decoder.Get ## fieldName());\
        auto& strRepr = values[decoder.Get ## fieldName()];\
        if (strRepr.size() > 2) {\
            StringSplitter(strRepr.substr(1, strRepr.size() - 2)).Split(',').SkipEmpty().Collect(&fieldName);\
        }\
    } else {\
        CHECK_WITH_LOG(!decoder.GetStrict()) << #fieldName << Endl;\
    }\
}
