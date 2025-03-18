#pragma once

#include "rule.h"

#include <antirobot/lib/enumerable_scanner.h>

#include <library/cpp/regex/pire/pire.h>


namespace NAntiRobot {


struct TPreparedRegexCondition {
    struct TGroupCondition {
        TCbbGroupId GroupId{};
        bool ShouldMatch = true;
    };

    TEnumerableScanner Scanner;
    TVector<TGroupCondition> GroupConditions;

    TPreparedRegexCondition() = default;

    explicit TPreparedRegexCondition(const TVector<TRegexCondition>& conditions);

    bool Empty() const {
        return Scanner.Empty() && GroupConditions.empty();
    }
};


struct TPreparedFactorCondition {
    size_t FactorIndex = 0;
    TConstantComparator<TInexactFloat3> Comparator;

    TPreparedFactorCondition() = default;

    explicit TPreparedFactorCondition(const TFactorCondition& condition);
};


struct TPreparedRule : public TRule {
    TPreparedRegexCondition PreparedDoc;
    TPreparedRegexCondition PreparedCgiString;
    TPreparedRegexCondition PreparedIdentType;
    TPreparedRegexCondition PreparedArrivalTime;
    TPreparedRegexCondition PreparedServiceType;
    TPreparedRegexCondition PreparedRequest;
    TPreparedRegexCondition PreparedHodor;
    TPreparedRegexCondition PreparedHodorHash;
    TPreparedRegexCondition PreparedJwsInfo;
    TPreparedRegexCondition PreparedYandexTrustInfo;
    TPreparedRegexCondition PreparedCountryId;

    THashMap<TString, TPreparedRegexCondition> PreparedHeaders;
    THashMap<TString, TPreparedRegexCondition> PreparedCsHeaders;

    TVector<TPreparedFactorCondition> PreparedFactors;

    TPreparedRule() = default;
    explicit TPreparedRule(TRule rule);

    static TPreparedRule Parse(const TString& s);

    static TVector<TPreparedRule> ParseList(
        const TVector<TString>& rules,
        TVector<TString>* errors = nullptr
    );
};


} // namespace NAntiRobot
