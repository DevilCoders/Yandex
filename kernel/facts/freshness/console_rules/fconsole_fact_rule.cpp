#include "fconsole_fact_rule.h"

#include <search/idl/meta.pb.h>

#include <util/generic/hash.h>
#include <util/stream/str.h>
#include <util/string/join.h>
#include <util/string/split.h>

namespace NFacts {

    constexpr char RULE_KEY_PREFIX[] = "rule_";
    constexpr char RULE_FIELDS_DELIMITER[] = "\t";

    void SerializeFConsoleRulesToReport(const TVector<TFConsoleFactRule>& rules, NMetaProtocol::TReport& dst) {
        TStringStream stringStream;
        for (size_t i = 0; i < rules.size(); ++i) {
            const auto& rule = rules[i];
            auto* const ruleProp = dst.AddSearcherProp();
            stringStream.Clear();
            stringStream << RULE_KEY_PREFIX << i << "_" << rule.TimestampSeconds;
            ruleProp->SetKey(stringStream.Str());

            stringStream.Clear();
            stringStream << rule.Type << RULE_FIELDS_DELIMITER << JoinSeq(RULE_FIELDS_DELIMITER, rule.Fields);
            ruleProp->SetValue(stringStream.Str());
        }
    }

    TVector<TVector<TString>> FetchFConsoleFactRules(const THashMap<TString, TString>& searcherProps) {
        TVector<TVector<TString>> rulesAsFields;
        for (const auto& kvPair : searcherProps) {
            if (!kvPair.first.StartsWith(RULE_KEY_PREFIX)) {
                continue;
            }
            TVector<TString> ruleFields;
            Split(kvPair.second, RULE_FIELDS_DELIMITER, ruleFields);
            if (ruleFields.size() > 0) {
                rulesAsFields.push_back(std::move(ruleFields));
            }
        }
        return rulesAsFields;
    }

}
