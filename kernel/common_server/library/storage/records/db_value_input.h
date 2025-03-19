#pragma once
#include <variant>
#include <library/cpp/json/writer/json_value.h>
#include <kernel/common_server/util/accessor.h>
#include <util/generic/guid.h>
#include <util/generic/utility.h>
#include "db_value.h"

namespace NCS {
    namespace NStorage {
        class TBinaryData {
        private:
            CSA_READONLY_DEF(TString, OriginalData);
            CSA_READONLY(bool, Deprecated, false);
            CSA_FLAG(TBinaryData, NullData, false);
            mutable TMaybe<TString> EncodedDataCache;

            TBinaryData() = default;
        public:
            static TBinaryData Null() {
                return TBinaryData().NullData();
            }

            TString GetEncodedData() const;

            explicit TBinaryData(const TString& str, const bool deprecated = false)
                : OriginalData(str)
                , Deprecated(deprecated) {
            }
            explicit TBinaryData(const char* str, const bool deprecated = false)
                : OriginalData(str)
                , Deprecated(deprecated) {
            }
            explicit TBinaryData(const TStringBuf str, const bool deprecated = false)
                : OriginalData(str)
                , Deprecated(deprecated) {
            }
        };

        using TDBValueInput = std::variant<bool, ui64, i64, ui32, i32, double, TString, TGUID, TBinaryData, TNull>;

        class TDBValueInputOperator {
        public:
            static NJson::TJsonValue SerializeToJson(const TDBValueInput& value);
            static TString SerializeToString(const TDBValueInput& value);

            static TDBValueInput ParseFromJson(const NJson::TJsonValue& jsonInfo);
            static TMaybe<TDBValue> ToDBValue(const TDBValueInput& input);
        };
    }
}
