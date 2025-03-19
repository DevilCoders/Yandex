#pragma once
#include "value_scanner.h"
#include <util/generic/set.h>
#include <util/generic/map.h>

namespace NCS {
    class TJsonScannerContext {
    private:
        TMap<TString, NJson::TJsonValue> OwnedValues;
        TMap<TString, const NJson::TJsonValue*> JsonValues;
    public:
        NJson::TJsonValue SerializeToJson() const {
            NJson::TJsonValue result = NJson::JSON_MAP;
            for (auto&& i : JsonValues) {
                result.SetValueByPath(i.first, *i.second);
            }
            return result;
        }

        size_t size() const {
            return JsonValues.size();
        }

        TMap<TString, const NJson::TJsonValue*>::const_iterator find(const TString& key) const {
            return JsonValues.find(key);
        }
        TMap<TString, const NJson::TJsonValue*>::const_iterator begin() const {
            return JsonValues.begin();
        }

        TMap<TString, const NJson::TJsonValue*>::const_iterator end() const {
            return JsonValues.end();
        }

        void Remove(const TStringBuf& key) {
            JsonValues.erase(TString(key));
            OwnedValues.erase(TString(key));
        }

        void Put(const TStringBuf& key, const NJson::TJsonValue* value) {
            JsonValues.emplace(key, value);
        }
        template <class T>
        void Put(const TStringBuf& key, const T value) {
            JsonValues.emplace(key, &OwnedValues.emplace(key, value).first->second);
        }
        const NJson::TJsonValue* Get(const TString& key) const {
            auto it = JsonValues.find(key);
            if (it == JsonValues.end()) {
                return nullptr;
            }
            return it->second;
        }
    };
    class TStackContext {
    private:
        THolder<IJsonConditionScanner> Scanner;
        TString MatchingExpression;
        TSet<TStringBuf> StoreValues;
        TJsonScannerContext& ContextInfo;
        bool InitContext();
        bool DropContext();
    public:
        TStackContext(const NJson::TJsonValue& value, TJsonScannerContext& contextInfo);

        bool Prepare(const TString& matchingContext);

        const NJson::TJsonValue& GetValue() const {
            return Scanner->GetValue();
        }

        bool IsValid() const {
            return Scanner->IsValid();
        }

        bool Next();
    };
}
