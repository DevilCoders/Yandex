#pragma once

#include <library/cpp/scheme/scheme.h>

namespace NUtil {

TString SerializeRearrangeParams(const NSc::TValue& scheme);

// not thread-safe
class TRearrangeOptionsHelper {
public:
    TRearrangeOptionsHelper(TString& rearrangeOptions);
    ~TRearrangeOptionsHelper();

    bool HasRule(const TString& name) const;
    bool AddRule(const TString& name, const TString& options = "");
    void EraseRule(const TString& name);
    TVector<TString> GetRules() const;

    NSc::TValue& RuleScheme(const TString& name);
private:
    using TRule = std::pair<TString, NSc::TValue>;
    using TRulesSchemes = TVector<TRule>;

private:
    TString& RearrangeOptions;
    TRulesSchemes RulesSchemes;
};
}
