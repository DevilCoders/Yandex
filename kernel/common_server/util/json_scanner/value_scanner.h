#pragma once
#include <library/cpp/json/writer/json_value.h>
#include <library/cpp/logger/global/global.h>
#include <util/generic/vector.h>
#include <util/generic/maybe.h>
#include <util/generic/set.h>
#include <util/generic/strbuf.h>
#include <kernel/common_server/util/accessor.h>

namespace NCS {
    class IJsonConditionScanner {
    protected:
        const NJson::TJsonValue& Value;

        virtual TString DoGetScannerKey() const = 0;
        virtual const NJson::TJsonValue& DoGetValue() const = 0;

    public:
        virtual ~IJsonConditionScanner() = default;

        IJsonConditionScanner(const NJson::TJsonValue& value)
            : Value(value) {

        }
        virtual TString GetScannerKey() const final {
            CHECK_WITH_LOG(IsValid());
            return DoGetScannerKey();
        }
        virtual const NJson::TJsonValue& GetValue() const final {
            CHECK_WITH_LOG(IsValid());
            return DoGetValue();
        }
        virtual bool IsValid() const = 0;
        virtual bool Next() = 0;
        virtual bool Prepare() = 0;
        virtual bool DeserializeFromString(const TStringBuf info) = 0;
    };

    class TArrayScanner: public IJsonConditionScanner {
    private:
        using TBase = IJsonConditionScanner;
        NJson::TJsonValue::TArray::const_iterator Iterator;
        TMaybe<ui32> Index;

        class TCondition {
        private:
            CSA_DEFAULT(TCondition, TStringBuf, Key);
            CSA_DEFAULT(TCondition, TSet<TStringBuf>, Values);
        public:
            TCondition() = default;
            bool DeserializeFromString(TStringBuf condition);
        };

        TVector<TCondition> Conditions;
        bool Check() const;
        virtual const NJson::TJsonValue& DoGetValue() const override {
            return *Iterator;
        }
        virtual TString DoGetScannerKey() const override {
            return ::ToString(Iterator - Value.GetArraySafe().begin());
        }
        bool AddCondition(TCondition&& c) {
            Conditions.emplace_back(std::move(c));
            return true;
        }
    public:
        TArrayScanner(const NJson::TJsonValue& value)
            : TBase(value) {
            CHECK_WITH_LOG(value.IsArray());
            Iterator = value.GetArraySafe().begin();
        }

        virtual bool Prepare() override {
            if (IsValid() && !Check()) {
                Next();
            }
            return true;
        }

        virtual bool IsValid() const override {
            return Iterator != Value.GetArraySafe().end();
        }
        virtual bool Next() override;

        virtual bool DeserializeFromString(const TStringBuf info) override;
    };

    class TMapScanner: public IJsonConditionScanner {
    private:
        using TBase = IJsonConditionScanner;
        NJson::TJsonValue::TMapType::const_iterator Iterator;

        TSet<TString> Indexes;
        bool Check() const {
            if (Indexes.size() && !Indexes.contains(Iterator->first)) {
                return false;
            }
            return true;
        }
        virtual const NJson::TJsonValue& DoGetValue() const override {
            return Iterator->second;
        }
        virtual TString DoGetScannerKey() const override {
            return Iterator->first;
        }
    public:
        TMapScanner(const NJson::TJsonValue& value)
            : TBase(value) {
            CHECK_WITH_LOG(value.IsMap());
            Iterator = Value.GetMapSafe().begin();
        }
        virtual bool IsValid() const override {
            return Iterator != Value.GetMapSafe().end();
        }
        virtual bool Next() override;

        virtual bool DeserializeFromString(const TStringBuf info) override;
        virtual bool Prepare() override {
            if (IsValid() && !Check()) {
                Next();
            }
            return true;
        }
    };

    class TValueScanner: public IJsonConditionScanner {
    private:
        using TBase = IJsonConditionScanner;

        ui32 idx = 0;
        virtual const NJson::TJsonValue& DoGetValue() const override {
            return Value;
        }
        virtual TString DoGetScannerKey() const override {
            return "";
        }
    public:
        virtual bool DeserializeFromString(const TStringBuf /*info*/) override {
            return true;
        }

        TValueScanner(const NJson::TJsonValue& value)
            : TBase(value) {
        }

        virtual bool Prepare() override {
            return true;
        }

        virtual bool IsValid() const override {
            return idx == 0;
        }
        virtual bool Next() override {
            ++idx;
            return idx == 1;
        }
    };

}
