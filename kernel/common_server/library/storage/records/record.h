#pragma once
#include <kernel/common_server/library/logging/events.h>
#include <library/cpp/json/writer/json_value.h>
#include <util/generic/fwd.h>
#include <util/generic/map.h>
#include <util/generic/set.h>
#include <util/string/cast.h>
#include <util/datetime/base.h>
#include <util/generic/guid.h>
#include <util/string/hex.h>
#include "abstract.h"
#include "db_value_input.h"

namespace NCS {
    namespace NStorage {
        namespace NRequest {
            class IExternalMethods;
        }

        class TTableRecord {
        public:
            void FilterColumns(const TSet<TString>& fieldIds) noexcept;
            TTableRecord FilterColumnsRet(const TSet<TString>& fieldIds, const bool fillNullAdditional = false) const noexcept;

            TMap<TString, TDBValueInput>::const_iterator begin() const noexcept {
                return Data.begin();
            }

            TMap<TString, TDBValueInput>::const_iterator end() const noexcept {
                return Data.end();
            }

            bool operator!() const noexcept {
                return !size();
            }

            size_t size() const noexcept {
                return Data.size();
            }

            bool IsContainsIn(const TTableRecord& r) const noexcept;
            bool IsSameFields(const TTableRecord& r) const noexcept;

            bool Empty() const noexcept {
                return Data.empty();
            }

            void RemoveOtherColumns(const TSet<TString>& columns) noexcept;

            TString GetKeys(const TString& separator = ", ") const noexcept;
            TString GetValues(const NRequest::IExternalMethods& transaction, const TSet<TString>* fieldsAll, const bool addEmptyStringOnNull = false) const noexcept;
            TString BuildSet(const NRequest::IExternalMethods& transaction) const noexcept;
            TString BuildCondition(const NRequest::IExternalMethods& transaction) const noexcept;

            TTableRecordWT BuildWT() const noexcept;

            template <class T>
            TTableRecord& SetNotEmpty(const TString& key, const T& value) noexcept {
                if (!!value) {
                    Set(key, value);
                }
                return *this;
            }

            TTableRecord& SetOrNull(const TString& key, const TString& value) noexcept {
                if (!!value) {
                    Set(key, value);
                } else {
                    Data.emplace(key, TNull());
                }
                return *this;
            }

            template <class T>
            TTableRecord& SetOrNull(const TString& key, const T& value) noexcept {
                if (!!value) {
                    Set(key, value);
                } else {
                    Data.emplace(key, TNull());
                }
                return *this;
            }

            TTableRecord& SetNull(const TString& key) noexcept {
                Data.emplace(key, TNull());
                return *this;
            }

            template <class T>
            TTableRecord& ForceSet(const TString& key, const T& value) noexcept {
                Data.erase(key);
                return Set(key, value);
            }

            template <class T>
            typename std::enable_if<std::is_enum<T>::value, TTableRecord>::type&
            Set(const TString& key, const T& value) noexcept {
                Data.emplace(key, ::ToString(value));
                return *this;
            }

            template <class T>
            typename std::enable_if<!std::is_enum<T>::value, TTableRecord>::type&
            Set(const TString& key, const T& value) noexcept {
                Data.emplace(key, value);
                return *this;
            }

            TTableRecord& Set(const TString& key, const TDBValue& value) noexcept;
            TTableRecord& Set(const TString& key, const ui32 value) noexcept {
                return Set<ui64>(key, value);
            }
            TTableRecord& Set(const TString& key, const i32 value) noexcept {
                return Set<i64>(key, value);
            }

            TTableRecord& Set(const TString& key, const NJson::TJsonValue& value) noexcept {
                return Set<TString>(key, value.GetStringRobust());
            }
            TTableRecord& Set(const TString& key, const TDuration value) noexcept {
                return Set<ui64>(key, value.Seconds());
            }
            TTableRecord& Set(const TString& key, const TInstant value) noexcept {
                return Set<ui64>(key, value.Seconds());
            }

            template <class T>
            TTableRecord& SetNotEmpty(const TString& key, const TMaybe<T>& value) noexcept {
                if (value) {
                    Set(key, *value);
                }
                return *this;
            }

            template <class TProto>
            TTableRecord& SetProtoDeprecated(const TString& key, const TProto& value) noexcept {
                try {
                    return SetBytesDeprecated(key, value.SerializePartialAsString());
                } catch (...) {
                    TFLEventLog::Alert("exception on SetProtoDeprecated")("message", CurrentExceptionMessage());
                    return SetBytesNull(key);
                }
            }

            template <class TProto>
            TTableRecord& SetProtoBytes(const TString& key, const TProto& value) noexcept {
                try {
                    return SetBytes(key, value.SerializePartialAsString());
                } catch (...) {
                    TFLEventLog::Alert("exception on SetProtoBytes")("message", CurrentExceptionMessage());
                    return SetBytesNull(key);
                }
            }

            template <class TProto>
            TTableRecord& SetProtoBytesPacked(const EDataCodec codec, const TString& key, const TProto& value) noexcept {
                try {
                    return SetBytesPacked(codec, key, value.SerializePartialAsString());
                } catch (...) {
                    TFLEventLog::Alert("exception on SetProtoBytesPacked")("message", CurrentExceptionMessage());
                    return SetBytesNull(key);
                }
            }

            template <class TProto>
            TTableRecord& SetProtoBytesPackedWithCodec(const EDataCodec codec, const TString& key, const TProto& value) noexcept {
                try {
                    return SetBytesPackedWithCodec(codec, key, value.SerializePartialAsString());
                } catch (...) {
                    TFLEventLog::Alert("exception on SetProtoBytesPackedWithCodec")("message", CurrentExceptionMessage());
                    return SetBytesNull(key);
                }
            }

            TTableRecord& SetBytesNull(const TString& key) noexcept;
            TTableRecord& SetBytes(const TString& key, const TStringBuf value) noexcept;
            TTableRecord& SetBytesPackedWithCodec(const EDataCodec codec, const TString& key, const TStringBuf value) noexcept;
            TTableRecord& SetBytesPacked(const EDataCodec codec, const TString& key, const TStringBuf value) noexcept;
            TTableRecord& SetBytesDeprecated(const TString& key, const TStringBuf value) noexcept;

            bool Has(const TString& key) const noexcept {
                return Data.contains(key);
            }

            TTableRecord& Remove(const TString& key) noexcept {
                auto it = Data.find(key);
                if (it != Data.end()) {
                    Data.erase(it);
                }
                return *this;
            }

            Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& json) noexcept;

            TString SerializeToString() const noexcept;
            TTableRecord SerializeToTableRecord() const;
            template <class T>
            TTableRecord& operator()(const TString& key, const T& value) noexcept {
                return Set(key, value);
            }

            TTableRecord() = default;

            template <class T>
            TTableRecord(const TString& key, const T& value) {
                Set(key, value);
            }
        private:
            TMap<TString, TDBValueInput> Data;
        };
    }
}
