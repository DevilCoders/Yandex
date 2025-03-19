#pragma once
#include <variant>
#include <library/cpp/json/writer/json_value.h>
#include <kernel/common_server/library/logging/events.h>
#include <util/generic/map.h>
#include "db_value.h"
#include "record.h"
#include "abstract.h"
#include <util/generic/maybe.h>
#include <util/generic/array_ref.h>
#include <library/cpp/json/json_reader.h>

namespace NCS {
    namespace NStorage {
        class ITransaction;
        class TSimpleDecoder;

        class TTableRecordWT {
        public:
            using TDBValue = NStorage::TDBValue;
        private:
            TMap<TString, TDBValue> Values;

            TString BuildConditionValue(const ITransaction& tr, const TDBValue& v) const noexcept;
        public:
            TTableRecordWT() = default;

            bool operator!() const noexcept {
                return !Values.size();
            }

            TString SerializeToString() const noexcept;

            TString BuildCondition(const ITransaction& transaction) const noexcept;

            bool ReadThroughDecoder(const TSimpleDecoder& decoder, const TConstArrayRef<TStringBuf>& values) noexcept;

            TSet<TString> GetKeys() const noexcept;

            TString CalcHashMD5(const bool normalKeys) const noexcept;

            NStorage::TTableRecord BuildTableRecord() const noexcept;

            void FilterColumns(const TSet<TString>& fieldIds) noexcept;
            TTableRecordWT FilterColumnsRet(const TSet<TString>& fieldIds) const noexcept;

            bool empty() const noexcept {
                return Values.empty();
            }

            size_t size() const noexcept {
                return Values.size();
            }

            TMap<TString, TDBValue>::const_iterator begin() const noexcept {
                return Values.begin();
            }

            TMap<TString, TDBValue>::const_iterator end() const noexcept {
                return Values.end();
            }

            TMaybe<TBlob> GetBlobDeprecated(const TString& key) const noexcept;
            TMaybe<TBlob> GetBlob(const TString& key) const noexcept;
            TMaybe<TString> GetBytesDeprecated(const TString& key) const noexcept;
            TMaybe<TString> GetBytes(const TString& key) const noexcept;

            TString GetString(const TString& key) const noexcept;
            TMaybe<TDBValue> GetValue(const TString& key) const noexcept;
            template <class T>
            TMaybe<T> CastTo(const TString& key) const noexcept {
                TString v = GetString(key);
                if (!v) {
                    return Nothing();
                }
                T value;
                if (!TryFromString(v, value)) {
                    return Nothing();
                }
                return value;
            }

            template <>
            TMaybe<TString> CastTo<TString>(const TString& key) const noexcept {
                return GetString(key);
            }

            template <>
            TMaybe<TInstant> CastTo<TInstant>(const TString& key) const noexcept {
                auto seconds = CastTo<ui32>(key);
                if (!seconds) {
                    return Nothing();
                }
                return TInstant::Seconds(*seconds);
            }

            template <>
            TMaybe<TDuration> CastTo<TDuration>(const TString& key) const noexcept {
                auto seconds = CastTo<ui32>(key);
                if (!seconds) {
                    return Nothing();
                }
                return TDuration::Seconds(*seconds);
            }

            template <>
            TMaybe<NJson::TJsonValue> CastTo<NJson::TJsonValue>(const TString& key) const noexcept {
                NJson::TJsonValue result;
                const TString v = GetString(key);
                if (!NJson::ReadJsonFastTree(v, &result)) {
                    return Nothing();
                }
                return result;
            }

            template <class T>
            TMaybe<T> GetStrict(const TString& key) const noexcept {
                TMaybe<TDBValue> v = GetValue(key);
                if (!v) {
                    return Nothing();
                }
                auto* vResult = std::get_if<T>(&*v);
                if (!vResult) {
                    return Nothing();
                }
                return *vResult;
            }

            template <class T>
            bool TryGet(const TString& key, T& result) const noexcept {
                auto resultData = CastTo<T>(key);
                if (!resultData) {
                    return false;
                }
                result = *resultData;
                return true;
            }

            template <class T>
            T TryGetDefault(const TString& key, const T& defValue) const noexcept {
                auto resultData = CastTo<T>(key);
                if (!resultData) {
                    return defValue;
                }
                return *resultData;
            }

            Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) noexcept;

            NJson::TJsonValue SerializeToJson() const noexcept;

            bool Set(const TString& key, const TStringBuf value, const EColumnType type, const bool nullable = false) noexcept;
            TTableRecordWT& Remove(const TString& key) noexcept;

            TTableRecordWT& Set(const TString& key, const TString& value) noexcept {
                Values[key] = value;
                return *this;
            }

            TTableRecordWT& Set(const TString& key, const TStringBuf value) noexcept {
                Values[key] = TString(value);
                return *this;
            }

            TTableRecordWT& Set(const TString& key, const TDBValue& value) noexcept {
                Values[key] = value;
                return *this;
            }

            TTableRecordWT& Set(const TString& key, const TInstant value) noexcept {
                Values[key] = value.Seconds();
                return *this;
            }

            TTableRecordWT& Set(const TString& key, const TDuration value) noexcept {
                Values[key] = value.Seconds();
                return *this;
            }

            template <class T>
            TTableRecordWT& Set(const TString& key, const T value) noexcept {
                Values[key] = value;
                return *this;
            }

            bool Has(const TString& key) const noexcept {
                return Values.contains(key);
            }
        };

    }
}
