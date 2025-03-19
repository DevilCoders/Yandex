#include "abstract.h"

namespace NCS {
    namespace NObfuscator {

        NFrontend::TScheme IObfuscatorWithRules::DoGetScheme(const IBaseServer& /*server*/) const {
            NFrontend::TScheme result;
            auto& matchRules = result.Add<TFSArray>("rules").SetElement<TFSStructure>().SetStructure();
            matchRules.Add<TFSString>("key");
            matchRules.Add<TFSString>("value");
            return result;
        }

        NJson::TJsonValue IObfuscatorWithRules::DoSerializeToJson() const {
            NJson::TJsonValue result = NJson::JSON_MAP;
            TMap<TString, TString> strRules;
            for (const auto& i : MatchRules) {
                strRules.emplace(i.first, JoinSeq(",", i.second));
            }
            TJsonProcessor::WriteMap(result, "rules", strRules);
            return result;
        }

        bool IObfuscatorWithRules::DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
            MatchRules.clear();
            TMap<TString, TString> strRules;
            if (!TJsonProcessor::ReadMap(jsonInfo, "rules", strRules)) {
                return false;
            }
            for (auto&& i : strRules) {
                TSet<TCiString> keys;
                StringSplitter(i.second).SplitBySet(", ").SkipEmpty().Collect(&keys);
                MatchRules.emplace(i.first, std::move(keys));
            }
            return true;
        }

        bool IObfuscatorWithRules::IsMatch(const TObfuscatorKey& obfuscatorKey) const {
            const auto* obfuscatorKeyMap = obfuscatorKey.GetAs<TObfuscatorKeyMap>();
            if (!obfuscatorKeyMap) {
                return false;
            }

            for (const auto& [key, value] : MatchRules) {
                auto it = obfuscatorKeyMap->GetParams().find(key);
                if (it == obfuscatorKeyMap->GetParams().end() || !value.contains(it->second)) {
                    return false;
                }
            }

            return true;
        }

    }
}
