#include "rearrange.h"

#include <search/web/util/config_parser/config_parser.h>

#include <util/generic/algorithm.h>

using namespace NUtil;

namespace {
    template <class TFirst>
    class TPairFindFirst {
    public:
        TPairFindFirst(const TFirst& what)
            : What(what)
        {}

        template <class TSecond>
        bool operator ()(const std::pair<TFirst, TSecond>& pair) const {
            return pair.first == What;
        }
    private:
        TFirst What;
    };

    const TString Dangerous = "{}:;=\'\"";

    inline TString GetSanitizedKey(const TStringBuf& s) {
        const TString string(s);
        if (string.find_first_of(Dangerous) != TString::npos) {
            return "\"" + string + "\"";
        } else {
            return string;
        }
    }
}

TRearrangeOptionsHelper::TRearrangeOptionsHelper(TString& rearrangeOptions)
    : RearrangeOptions(rearrangeOptions)
{
    NRearr::TSimpleConfiguration rulesConfig = NRearr::ParseSimpleOptions(RearrangeOptions,
                                                                          NRearr::TDebugInfo("ReArrangeOptions", NRearr::LogError));

    for (const auto& config : rulesConfig.GetRules()) {
        RulesSchemes.push_back(std::make_pair(config.GetName(), config.GetScheme()));
    }
}

TRearrangeOptionsHelper::~TRearrangeOptionsHelper() {
    RearrangeOptions = "";
    for (ui32 i = 0; i < RulesSchemes.size(); ++i) {
        if (i > 0) {
            RearrangeOptions += ' ';
        }
        RearrangeOptions += RulesSchemes[i].first;

        const NSc::TValue& scheme = RulesSchemes[i].second;
        if (!scheme.ArrayEmpty() || !scheme.DictEmpty() || !scheme.StringEmpty()) {
            RearrangeOptions += "(" + SerializeRearrangeParams(scheme) + ')';
        }
    }
}

bool TRearrangeOptionsHelper::HasRule(const TString& name) const {
    return FindIf(RulesSchemes.begin(), RulesSchemes.end(), TPairFindFirst<TString>(name)) != RulesSchemes.end();
}

bool TRearrangeOptionsHelper::AddRule(const TString& name, const TString& options /*= ""*/) {
    using namespace NRearr;

    if (HasRule(name)) {
        return false;
    }

    RulesSchemes.push_back(std::make_pair(name, FillSchemeFromOptions(options, TDebugInfo(name, LogError))));
    return true;
}

void TRearrangeOptionsHelper::EraseRule(const TString& name) {
    auto rule = FindIf(RulesSchemes.begin(), RulesSchemes.end(), TPairFindFirst<TString>(name));
    if (rule == RulesSchemes.end())
        throw yexception() << "rule " << name << " is not found";
    RulesSchemes.erase(rule);
}

NSc::TValue& TRearrangeOptionsHelper::RuleScheme(const TString& name) {
    auto rule = FindIf(RulesSchemes.begin(), RulesSchemes.end(), TPairFindFirst<TString>(name));
    if (rule == RulesSchemes.end())
        throw yexception() << "rule " << name << " is not found";
    return rule->second;
}

TVector<TString> NUtil::TRearrangeOptionsHelper::GetRules() const {
    TVector<TString> result;
    for (ui32 i = 0; i < RulesSchemes.size(); ++i) {
        result.push_back(RulesSchemes[i].first);
    }
    return result;
}

TString NUtil::SerializeRearrangeParams(const NSc::TValue& scheme) {
    if (scheme.IsString()) {
        return GetSanitizedKey(scheme.GetString());
    }

    if (scheme.IsDict()) {
        TString result;
        for (auto&& i : scheme.GetDict()) {
            if (result) {
                result += ", ";
            }
            result += GetSanitizedKey(i.first);
            result += '=';
            result += i.second.ToJson(NSc::TValue::JO_DEFAULT);
        }
        return result;
    }

    return TString();
}
