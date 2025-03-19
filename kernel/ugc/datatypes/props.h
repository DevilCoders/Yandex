#pragma once

#include <library/cpp/scheme/scheme.h>

#include <util/generic/map.h>
#include <util/generic/string.h>
#include <util/generic/strbuf.h>

// NOTE: constructors and factory functions here throw TBadArgumentException
//       in case of parse error.

namespace NUgc {
    class TVirtualRow {
    public:
        using TProps = TMap<TString, TString>;

    public:
        // Sets or replace prop
        TVirtualRow& SetProp(const TString& propKey, const TString& propValue);

        // Sets prop value to empty string so that it forms update that "removes" prop
        TVirtualRow& RemoveProp(const TString& propKey);

        // Merges props
        TVirtualRow& Merge(const TVirtualRow& row);

        // Gets existing prop value or returns ""
        TString GetProp(const TString& propKey) const;

        // Checks whether row has prop value with specified key and its value is not empty
        bool HasProp(const TString& propKey) const;

        const TProps& GetProps() const;

        // Deserialization
        static TVirtualRow FromJson(const TStringBuf& json) {
            return FromJson(NSc::TValue::FromJsonThrow(json));
        }

        static TVirtualRow FromJson(const TString& json) {
            return FromJson(NSc::TValue::FromJsonThrow(json));
        }

        static TVirtualRow FromJson(const char* json) {
            return FromJson(NSc::TValue::FromJsonThrow(json));
        }

        static TVirtualRow FromJson(const NSc::TValue& json);

        // Serialization
        NSc::TValue ToJson() const;

    private:
        TProps Props;
    };

    class TVirtualTable {
    public:
        using TRows = TMap<TString, TVirtualRow>;

    public:
        // Gives existing row or creates new one
        TVirtualRow& MutableRow(const TString& key);

        // Gets existing row or returns empty one
        const TVirtualRow& GetRow(const TString& key) const;

        // Removes row
        TVirtualTable& RemoveRow(const TString& key);

        // Checks row existense
        bool HasRow(const TString& key) const;

        // Deserialization
        static TVirtualTable FromJson(const TStringBuf& json) {
            return FromJson(NSc::TValue::FromJsonThrow(json));
        }

        static TVirtualTable FromJson(const TString& json) {
            return FromJson(NSc::TValue::FromJsonThrow(json));
        }

        static TVirtualTable FromJson(const char* json) {
            return FromJson(NSc::TValue::FromJsonThrow(json));
        }

        static TVirtualTable FromJson(const NSc::TValue& json);

        // Serialization
        NSc::TValue ToJson() const;

        const TRows& GetRows() const;

    private:
        TRows Rows;
    };

    class TVirtualTables {
    public:
        using TTables = TMap<TString, TVirtualTable>;

    public:
        TTables::iterator begin() {
            return Tables.begin();
        }

        TTables::iterator end() {
            return Tables.end();
        }

        // Gives existing table or creates new one
        TVirtualTable& MutableTable(const TString& name);

        // Gets existing table or returns empty one
        const TVirtualTable& GetTable(const TString& name) const;

        // Removes table
        TVirtualTables& RemoveTable(const TString& name);

        // Checks table existense
        bool HasTable(const TString& name) const;

        // Deserialization
        static TVirtualTables FromJson(const TStringBuf& json) {
            return FromJson(NSc::TValue::FromJsonThrow(json));
        }

        static TVirtualTables FromJson(const TString& json) {
            return FromJson(NSc::TValue::FromJsonThrow(json));
        }

        static TVirtualTables FromJson(const char* json) {
            return FromJson(NSc::TValue::FromJsonThrow(json));
        }

        static TVirtualTables FromJson(const NSc::TValue& json);

        // Serialization
        NSc::TValue ToJson() const;

    private:
        TTables Tables;
    };
} // namespace NUgc
