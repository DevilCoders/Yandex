#include "create_table.h"
#include <kernel/common_server/library/storage/records/record.h>
#include <kernel/common_server/library/storage/records/t_record.h>

namespace NCS {
    namespace NStorage {
        NCS::NStorage::TColumnInfo& TCreateTableQuery::AddColumn(const TString& columnId) {
            Columns.emplace_back(columnId);
            return Columns.back();
        }

        bool TColumnInfo::NeedQuote() const {
            switch (Type) {
                case EColumnType::Double:
                case EColumnType::I32:
                case EColumnType::I64:
                case EColumnType::UI64:
                case EColumnType::Boolean:
                    return false;
                case EColumnType::Text:
                case EColumnType::Binary:
                    return true;
            }
        }

        bool TColumnInfo::IsCorrectValue(const TString& value) const {
            return !!GetDBValue(value);
        }

        TMaybe<TDBValue> TColumnInfo::GetDBValue(const TString& value) const {
            return TDBValueOperator::ParseString(value, Type);
        }

        NCS::NScheme::TScheme TColumnInfo::GetScheme() {
            NCS::NScheme::TScheme result;
            result.Add<NCS::NScheme::TFSString>("id").SetNonEmpty(true);
            result.Add<NCS::NScheme::TFSBoolean>("nullable").SetDefault(true);
            result.Add<NCS::NScheme::TFSBoolean>("autogeneration").SetDefault(false);
            result.Add<NCS::NScheme::TFSBoolean>("primary").SetDefault(false);
            result.Add<NCS::NScheme::TFSBoolean>("unique").SetDefault(false);
            result.Add<NCS::NScheme::TFSVariants>("type").InitVariants<EColumnType>().SetDefault(::ToString(EColumnType::Text));
            return result;
        }

        bool TColumnInfo::WriteValue(TTableRecordWT& tr, const TString& value) const {
            auto dbValue = GetDBValue(value);
            if (!dbValue) {
                return false;
            }
            tr.Set(Id, *dbValue);
            return true;
        }

        bool TColumnInfo::WriteValue(TTableRecord& tr, const TString& value) const {
            auto dbValue = GetDBValue(value);
            if (!dbValue) {
                return false;
            }
            tr.Set(Id, *dbValue);
            return true;
        }

        bool TColumnInfo::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
            if (!TJsonProcessor::Read(jsonInfo, "id", Id, true, false)) {
                return false;
            }
            Id = ::ToLowerUTF8(Id);
            if (!TJsonProcessor::Read(jsonInfo, "nullable", NullableFlag)) {
                return false;
            }
            if (!TJsonProcessor::Read(jsonInfo, "autogeneration", AutogenerationFlag)) {
                return false;
            }
            if (!TJsonProcessor::Read(jsonInfo, "primary", PrimaryFlag)) {
                return false;
            }
            if (!TJsonProcessor::Read(jsonInfo, "unique", UniqueFlag)) {
                return false;
            }
            if (!TJsonProcessor::ReadFromString(jsonInfo, "type", Type)) {
                return false;
            }
            return true;
        }

        NJson::TJsonValue TColumnInfo::SerializeToJson() const {
            NJson::TJsonValue result = NJson::JSON_MAP;
            TJsonProcessor::Write(result, "id", Id);
            TJsonProcessor::Write(result, "nullable", NullableFlag);
            TJsonProcessor::Write(result, "autogeneration", AutogenerationFlag);
            TJsonProcessor::Write(result, "primary", PrimaryFlag);
            TJsonProcessor::Write(result, "unique", UniqueFlag);
            TJsonProcessor::WriteAsString(result, "type", Type);
            return result;
        }

    }
}
