#include "record.h"
#include <util/string/hex.h>
#include <kernel/common_server/util/algorithm/iterator.h>
#include <kernel/common_server/library/storage/query/request.h>
#include <library/cpp/json/json_reader.h>
#include <util/string/escape.h>
#include <util/system/yassert.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/blockcodecs/core/register.h>
#include <kernel/common_server/library/storage/proto/packed_data.pb.h>

template <>
void Out<NCS::NStorage::TTableRecord>(IOutputStream& out, const NCS::NStorage::TTableRecord& value) {
    out << value.SerializeToString();
}

namespace NCS {
    namespace NStorage {
        void TTableRecord::FilterColumns(const TSet<TString>& fieldIds) noexcept {
            auto itField = fieldIds.begin();
            for (auto it = Data.begin(); it != Data.end(); ) {
                if (AdvanceSimple(itField, fieldIds.end(), it->first)) {
                    ++it;
                } else {
                    it = Data.erase(it);
                }
            }
        }

        NCS::NStorage::TTableRecord TTableRecord::FilterColumnsRet(const TSet<TString>& fieldIds, const bool fillNullAdditional) const noexcept {
            TTableRecord result;
            auto it = Data.begin();
            for (auto&& i : fieldIds) {
                if (Advance(it, Data.end(), i)) {
                    result.Set(i, it->second);
                } else if (fillNullAdditional) {
                    result.SetNull(i);
                }
            }
            return result;
        }

        bool TTableRecord::IsSameFields(const TTableRecord& r) const noexcept {
            if (r.Data.size() != Data.size()) {
                return false;
            }
            auto itR = r.Data.begin();
            for (auto it = Data.begin(); it != Data.end(); ++it) {
                if (!Advance(itR, r.Data.end(), it->first)) {
                    return false;
                }
            }
            return true;
        }

        void TTableRecord::RemoveOtherColumns(const TSet<TString>& columns) noexcept {
            for (auto it = Data.begin(); it != Data.end(); ) {
                if (columns.contains(it->first)) {
                    ++it;
                } else {
                    it = Data.erase(it);
                }
            }
        }

        TString TTableRecord::GetKeys(const TString& separator) const noexcept {
            TStringStream keys;
            for (auto&& i : Data) {
                if (!keys.Empty()) {
                    keys << separator;
                }
                keys << i.first;
            }
            return keys.Str();
        }

        enum class EProcessValueType {
            Json,
            Value,
        };

        NCS::NStorage::TTableRecord& TTableRecord::SetBytes(const TString& key, const TStringBuf value) noexcept {
            TBinaryData bData(value);
            Set(key, bData);
            return *this;
        }

        NCS::NStorage::TTableRecord& TTableRecord::SetBytesNull(const TString& key) noexcept {
            Set(key, TBinaryData::Null());
            return *this;
        }

        TTableRecord& TTableRecord::SetBytesPackedWithCodec(const EDataCodec codec, const TString& key, const TStringBuf value) noexcept {
            NCSProto::TPackedData proto;
            if (codec != EDataCodec::Null) {
                proto.SetCodec((ui32)codec);
            }
            try {
                const NBlockCodecs::ICodec* c = NBlockCodecs::Codec(::ToString(codec));
                CHECK_WITH_LOG(c);
                proto.SetData(c->Encode(value));
            } catch (...) {
                TFLEventLog::Alert("cannot fill packed bytes")("codec", codec);
            }
            return SetBytes(key, proto.SerializeAsString());
        }

        TTableRecord& TTableRecord::SetBytesPacked(const EDataCodec codec, const TString& key, const TStringBuf value) noexcept {
            TString packedData;
            try {
                const NBlockCodecs::ICodec* c = NBlockCodecs::Codec(::ToString(codec));
                CHECK_WITH_LOG(c);
                packedData = c->Encode(value);
            } catch (...) {
                TFLEventLog::Alert("cannot fill packed bytes")("codec", codec);
            }
            return SetBytes(key, packedData);
        }

        NCS::NStorage::TTableRecord& TTableRecord::SetBytesDeprecated(const TString& key, const TStringBuf value) noexcept {
            TBinaryData bData(value, true);
            Set(key, bData);
            return *this;
        }

        bool TTableRecord::DeserializeFromJson(const NJson::TJsonValue& json) noexcept {
            Data.clear();
            NJson::TJsonValue::TMapType jsonMap;
            if (!json.GetMap(&jsonMap)) {
                return false;
            }
            for (auto&& i : jsonMap) {
                if (!Data.emplace(i.first, TDBValueInputOperator::ParseFromJson(i.second)).second) {
                    TFLEventLog::Error("field duplication")("field_id", i.first);
                    return false;
                }
            }
            return true;
        }

        TString TTableRecord::SerializeToString() const noexcept {
            TStringBuilder sb;
            for (auto&& i : Data) {
                sb << i.first << "=" << EscapeC(TDBValueInputOperator::SerializeToString(i.second)) << ";";
            }
            return sb;
        }

        NCS::NStorage::TTableRecord TTableRecord::SerializeToTableRecord() const {
            return *this;
        }

        namespace {
            class TCompositeIterator {
            private:
                using TFieldsIt = TSet<TString>::const_iterator;
                using TDataIt = TMap<TString, TDBValueInput>::const_iterator;
                const bool AddEmptyStringOnNull = false;
                TFieldsIt ItFull;
                TFieldsIt EndFull;
                mutable TDataIt ItData;
                mutable TDataIt EndData;
            public:
                TCompositeIterator(const TSet<TString>& fields, const TMap<TString, TDBValueInput>& data, const bool addEmptyStringOnNull)
                    : AddEmptyStringOnNull(addEmptyStringOnNull)
                    , ItFull(fields.begin())
                    , EndFull(fields.end())
                    , ItData(data.begin())
                    , EndData(data.end()) {

                }

                void Next() {
                    ++ItFull;
                }

                bool IsValid() const {
                    return ItFull != EndFull;
                }

                TDBValueInput GetValue() const {
                    TDBValueInput result = TNull();
                    if (Advance(ItData, EndData, *ItFull)) {
                        result = ItData->second;
                    }
                    if (std::get_if<TNull>(&result) && AddEmptyStringOnNull) {
                        return "";
                    } else {
                        return result;
                    }
                }
            };

            class TSimpleIterator {
            private:
                TMap<TString, TDBValueInput>::const_iterator ItData;
                TMap<TString, TDBValueInput>::const_iterator EndData;
            public:
                TSimpleIterator(const TMap<TString, TDBValueInput>& data)
                    : ItData(data.begin())
                    , EndData(data.end()) {

                }

                void Next() {
                    ++ItData;
                }

                bool IsValid() const {
                    return ItData != EndData;
                }

                TDBValueInput GetValue() const {
                    return ItData->second;
                }
            };

            template <class TIterator>
            TString BuildValues(TIterator& iterator, const NRequest::IExternalMethods& transaction) noexcept {
                TStringBuilder request;
                for (; iterator.IsValid(); iterator.Next()) {
                    if (!request.Empty()) {
                        request << ", ";
                    }
                    request << transaction.Quote(iterator.GetValue());
                }
                return request;
            }

        }

        TString TTableRecord::GetValues(const NRequest::IExternalMethods& transaction, const TSet<TString>* fieldsAll, const bool addEmptyStringOnNull) const noexcept {
            const bool needRemap = fieldsAll && (fieldsAll->size() != Data.size());
            if (needRemap) {
                TCompositeIterator it(*fieldsAll, Data, addEmptyStringOnNull);
                return BuildValues(it, transaction);
            } else {
                TSimpleIterator it(Data);
                return BuildValues(it, transaction);
            }
        }

        TString TTableRecord::BuildSet(const NRequest::IExternalMethods& transaction) const noexcept {
            TStringStream request;
            for (auto&& i : Data) {
                if (!request.Empty()) {
                    request << ", ";
                }
                request << i.first << "=" << transaction.Quote(i.second);
            }
            return request.Str();
        }

        TString TTableRecord::BuildCondition(const NRequest::IExternalMethods& transaction) const noexcept {
            TStringStream request;
            for (auto&& i : Data) {
                if (!request.Empty()) {
                    request << " AND ";
                }
                const TString conditionValue = transaction.Quote(i.second);
                bool requireEqualitySignCompare = (conditionValue != "null" && conditionValue != "not null");
                request << i.first << ((requireEqualitySignCompare) ? "=" : " is ") << conditionValue;
            }
            return request.Str();
        }

        NCS::NStorage::TTableRecordWT TTableRecord::BuildWT() const noexcept {
            TTableRecordWT result;
            for (auto&& i : Data) {
                TMaybe<TDBValue> dbObject = TDBValueInputOperator::ToDBValue(i.second);
                if (!dbObject) {
                    continue;
                }
                result.Set(i.first, *dbObject);
            }
            return result;
        }

        NCS::NStorage::TTableRecord& TTableRecord::Set(const TString& key, const TDBValue& value) noexcept {
            const auto pred = [this, &key](const auto& v) {
                Set(key, v);
            };
            std::visit(pred, value);
            return *this;
        }

    }
}
