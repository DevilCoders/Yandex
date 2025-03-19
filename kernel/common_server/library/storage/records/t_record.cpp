#include "t_record.h"
#include <kernel/common_server/util/algorithm/iterator.h>
#include <kernel/common_server/library/logging/events.h>
#include <library/cpp/digest/md5/md5.h>
#include <kernel/common_server/util/algorithm/container.h>
#include <kernel/common_server/library/storage/reply/decoder.h>
#include <kernel/common_server/library/storage/abstract/transaction.h>

namespace NCS {
    namespace NStorage{

        TString TTableRecordWT::BuildConditionValue(const ITransaction& tr, const TDBValue& v) const noexcept {
            const auto pred = [&tr](const auto& v) {
                return tr.Quote(v);
            };
            return std::visit(pred, v);
        }

        TString TTableRecordWT::SerializeToString() const noexcept {
            TStringBuilder sb;
            for (auto&& i : Values) {
                sb << i.first << "=" << EscapeC(TDBValueOperator::SerializeToString(i.second)) << ";";
            }
            return sb;
        }

        TString TTableRecordWT::BuildCondition(const ITransaction& transaction) const noexcept {
            TStringStream request;
            for (auto&& i : Values) {
                if (!request.Empty()) {
                    request << " AND ";
                }
                TString conditionValue = BuildConditionValue(transaction, i.second);
                bool requireEqualitySignCompare = (conditionValue != "null" && conditionValue != "not null");
                request << i.first << ((requireEqualitySignCompare) ? "=" : " is ") << conditionValue;
            }
            return request.Str();
        }

        bool TTableRecordWT::ReadThroughDecoder(const TSimpleDecoder& decoder, const TConstArrayRef<TStringBuf>& values) noexcept {
            for (ui32 i = 0; i < decoder.GetColumns().size(); ++i) {
                if (i < (ui32)values.size()) {
                    if (!Set(decoder.GetColumns()[i].GetName(), values[i], decoder.GetColumns()[i].GetType(), true)) {
                        TFLEventLog::Error("incompatible types")("index", i)("column_id", decoder.GetColumns()[i].GetName())("expected_type", decoder.GetColumns()[i].GetType())("real_value", values[i]);
                        return false;
                    }
                } else {
                    TFLEventLog::Error("incompatible value index")("index", i)("column_id", decoder.GetColumns()[i].GetName());
                    return false;
                }
            }
            return true;
        }

        TSet<TString> TTableRecordWT::GetKeys() const noexcept {
            return MakeSet(NContainer::Keys(Values));
        }

        TString TTableRecordWT::CalcHashMD5(const bool normalKeys) const noexcept {
            TStringBuilder sb;
            for (auto&& i : Values) {
                sb << (normalKeys ? ::ToLowerUTF8(i.first) : i.first) << ":" << TDBValueOperator::SerializeToString(i.second) << Endl;
            }
            return MD5::Calc(sb);
        }

        NStorage::TTableRecord TTableRecordWT::BuildTableRecord() const noexcept {
            NStorage::TTableRecord result;
            for (auto&& i : Values) {
                const TString& name = i.first;
                const auto pred = [&result, &name](const auto& r) {
                    result.Set(name, r);
                };
                std::visit(pred, i.second);
            }
            return result;
        }

        void TTableRecordWT::FilterColumns(const TSet<TString>& fieldIds) noexcept {
            auto itField = fieldIds.begin();
            for (auto it = Values.begin(); it != Values.end(); ) {
                if (AdvanceSimple(itField, fieldIds.end(), it->first)) {
                    ++it;
                } else {
                    it = Values.erase(it);
                }
            }
        }

        NCS::NStorage::TTableRecordWT TTableRecordWT::FilterColumnsRet(const TSet<TString>& fieldIds) const noexcept {
            TTableRecordWT result;
            for (auto&& i : Values) {
                if (!fieldIds.contains(i.first)) {
                    continue;
                }
                result.Values.emplace(i.first, i.second);
            }
            return result;
        }

        TMaybe<TDBValue> TTableRecordWT::GetValue(const TString& key) const noexcept {
            auto it = Values.find(key);
            if (it == Values.end()) {
                return Nothing();
            } else {
                return it->second;
            }
        }

        TMaybe<TBlob> TTableRecordWT::GetBlobDeprecated(const TString& key) const noexcept {
            auto result = GetBytesDeprecated(key);
            if (!result) {
                return Nothing();
            } else {
                return TBlob::FromString(std::move(*result));
            }
        }

        TMaybe<TBlob> TTableRecordWT::GetBlob(const TString& key) const noexcept {
            auto result = GetBytes(key);
            if (!result) {
                return Nothing();
            } else {
                return TBlob::FromString(std::move(*result));
            }
        }

        TMaybe<TString> TTableRecordWT::GetBytesDeprecated(const TString& key) const noexcept {
            TMaybe<TDBValue> resultData = GetValue(key);
            if (!resultData) {
                return "";
            }
            const TString bData = TDBValueOperator::SerializeToString(*resultData);
            try {
                return Base64Decode(bData);
            } catch (...) {
                return Nothing();
            }
        }

        TMaybe<TString> TTableRecordWT::GetBytes(const TString& key) const noexcept {
            return GetString(key);
        }

        TString TTableRecordWT::GetString(const TString& key) const noexcept {
            auto it = Values.find(key);
            if (it == Values.end()) {
                return "";
            } else {
                return TDBValueOperator::SerializeToString(it->second);
            }
        }

        bool TTableRecordWT::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) noexcept {
            TMap<TString, TDBValue> values;
            NJson::TJsonValue::TMapType jsonMap;
            if (!jsonInfo.GetMap(&jsonMap)) {
                return false;
            }
            for (auto&& i : jsonMap) {
                TDBValue v;
                if (!TDBValueOperator::DeserializeFromJson(v, i.second)) {
                    return false;
                }
                values.emplace(i.first, std::move(v));
            }
            std::swap(values, Values);
            return true;
        }

        NJson::TJsonValue TTableRecordWT::SerializeToJson() const noexcept {
            NJson::TJsonValue result = NJson::JSON_MAP;
            for (auto&& i : Values) {
                result.InsertValue(i.first, TDBValueOperator::SerializeToJson(i.second));
            }
            return result;
        }

        TTableRecordWT& TTableRecordWT::Remove(const TString& key) noexcept {
            Values.erase(key);
            return *this;
        }

        bool TTableRecordWT::Set(const TString& key, const TStringBuf value, const EColumnType type, const bool nullable) noexcept {
            if (type == EColumnType::Text) {
                Set(key, value);
                return true;
            }
            if (nullable && !value) {
                return true;
            }
            auto dbValue = TDBValueOperator::ParseString(value, type);
            if (!dbValue) {
                return false;
            }
            Set(key, *dbValue);
            return true;
        }

    }
}
