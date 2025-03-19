#include "props.h"

#include <util/generic/hash_set.h>
#include <util/generic/is_in.h>
#include <util/generic/singleton.h>

namespace NUgc {
    const TString KEY = "key";

    const THashSet<TString> RESERVED_TABLE_NAMES = {
        "version",
        "type",
        "userId",
        "visitorId",
        "deviceId",
        "updateId",
        "appId",
        "app",
        "time",
        "timeFrom",
        "timeTo",
        "yandexTld",
        "countryCode",
    };

#define VALIDATE_PROP_KEY(propKey) Y_ENSURE_EX(propKey != KEY, TBadArgumentException() << "Prop [" << KEY << "] is reserved")

#define VALIDATE_TABLE_NAME(name) Y_ENSURE_EX(!IsIn(RESERVED_TABLE_NAMES, name), TBadArgumentException() << "Table name [" << name << "] is reserved")

    TVirtualRow& TVirtualRow::SetProp(const TString& propKey, const TString& propValue) {
        VALIDATE_PROP_KEY(propKey);
        Props[propKey] = propValue;
        return *this;
    }

    TVirtualRow& TVirtualRow::RemoveProp(const TString& propKey) {
        // SetProp() validates key name
        return SetProp(propKey, TString());
    }

    TVirtualRow& TVirtualRow::Merge(const TVirtualRow& row) {
        for (const auto& prop : row.Props) {
            Props[prop.first] = prop.second;
        }
        return *this;
    }

    TString TVirtualRow::GetProp(const TString& propKey) const {
        VALIDATE_PROP_KEY(propKey);
        const auto found = Props.find(propKey);
        return found == Props.end() ? TString() : found->second;
    }

    bool TVirtualRow::HasProp(const TString& propKey) const {
        VALIDATE_PROP_KEY(propKey);
        const auto found = Props.find(propKey);
        return found != Props.end() && !found->second.empty();
    }

    const TVirtualRow::TProps& TVirtualRow::GetProps() const {
        return Props;
    }

    TVirtualRow TVirtualRow::FromJson(const NSc::TValue& json) {
        TVirtualRow row;
        Y_ENSURE_EX(json.IsDict(), TBadArgumentException());
        for (const auto& kv : json.GetDict()) {
            if (kv.first != KEY) {
                Y_ENSURE_EX(!kv.second.IsDict() && !kv.second.IsArray(), TBadArgumentException() << "Key [" << kv.first << "] has non simple type: " << kv.second << ".");
                row.SetProp(TString(kv.first), TString(kv.second.ForceString()));
            }
        }
        return row;
    }

    NSc::TValue TVirtualRow::ToJson() const {
        NSc::TValue val;
        for (const auto& prop : Props) {
            val[prop.first] = prop.second;
        }
        return val;
    }

    TVirtualRow& TVirtualTable::MutableRow(const TString& key) {
        return Rows[key];
    }

    const TVirtualRow& TVirtualTable::GetRow(const TString& key) const {
        const auto found = Rows.find(key);
        return found == Rows.end() ? Default<TVirtualRow>() : found->second;
    }

    const TVirtualTable::TRows& TVirtualTable::GetRows() const {
        return Rows;
    }

    TVirtualTable& TVirtualTable::RemoveRow(const TString& key) {
        Rows.erase(key);
        return *this;
    }

    bool TVirtualTable::HasRow(const TString& key) const {
        return Rows.find(key) != Rows.end();
    }

    TVirtualTable TVirtualTable::FromJson(const NSc::TValue& json) {
        TVirtualTable table;
        Y_ENSURE_EX(json.IsArray(), TBadArgumentException());
        for (const NSc::TValue& row : json.GetArray()) {
            const NSc::TValue& key = row[KEY];
            Y_ENSURE_EX(key.IsString(), TBadArgumentException() << KEY << " must be string: " << key);
            const TString strKey(key.GetString());
            try {
                table.MutableRow(strKey).Merge(TVirtualRow::FromJson(row));
            } catch (const TBadArgumentException&) {
                ythrow TBadArgumentException() << "Failed to parse row [" << strKey << "]: " << CurrentExceptionMessage();
            }
        }
        return table;
    }

    NSc::TValue TVirtualTable::ToJson() const {
        NSc::TValue val;
        val.SetArray();
        for (const auto& row : Rows) {
            NSc::TValue jsonRow = row.second.ToJson();
            jsonRow[KEY] = row.first;
            val.Push(jsonRow);
        }
        return val;
    }

    TVirtualTable& TVirtualTables::MutableTable(const TString& name) {
        VALIDATE_TABLE_NAME(name);
        return Tables[name];
    }

    const TVirtualTable& TVirtualTables::GetTable(const TString& name) const {
        VALIDATE_TABLE_NAME(name);
        const auto found = Tables.find(name);
        return found == Tables.end() ? Default<TVirtualTable>() : found->second;
    }

    TVirtualTables& TVirtualTables::RemoveTable(const TString& name) {
        VALIDATE_TABLE_NAME(name);
        Tables.erase(name);
        return *this;
    }

    bool TVirtualTables::HasTable(const TString& name) const {
        VALIDATE_TABLE_NAME(name);
        return Tables.find(name) != Tables.end();
    }

    TVirtualTables TVirtualTables::FromJson(const NSc::TValue& json) {
        Y_ENSURE_EX(json.IsDict(), TBadArgumentException() << "Tables json must be dict");
        TVirtualTables tables;
        for (const auto& table : json.GetDict()) {
            if (!IsIn(RESERVED_TABLE_NAMES, table.first)) {
                try {
                    tables.Tables.insert(std::make_pair(table.first, TVirtualTable::FromJson(table.second)));
                } catch (const TBadArgumentException&) {
                    ythrow TBadArgumentException() << "Failed to parse table [" << table.first << "]: " << CurrentExceptionMessage();
                }
            }
        }
        return tables;
    }

    NSc::TValue TVirtualTables::ToJson() const {
        NSc::TValue val;
        for (const auto& table : Tables) {
            val[table.first] = table.second.ToJson();
        }
        return val;
    }
} // namespace NUgc
