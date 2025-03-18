#include "prepared_rule.h"

#include "cacher_factors.h"
#include "match_rule_parser.h"

#include <util/string/builder.h>

#include <exception>


namespace NAntiRobot {


namespace {


TEnumerableScanner CompileFsms(
    TVector<std::pair<NPire::TFsm, bool>>& fsms
) {
    if (fsms.empty()) {
        return TEnumerableScanner();
    }

    if (fsms.size() == 1) {
        auto& [fsm, shouldMatch] = fsms[0];
        if (!shouldMatch) {
            fsm.Complement();
        }

        return TEnumerableScanner(fsm);
    }

    for (auto& [fsm, shouldMatch] : fsms) {
        if (shouldMatch) {
            fsm.Complement();
        }
    }

    size_t numFsms = fsms.size();
    while (numFsms > 1) {
        for (size_t i = 0; i + 1 < numFsms; i += 2) {
            fsms[i].first |= fsms[i + 1].first;
            if (i / 2 != i) {
                fsms[i  / 2].first = std::move(fsms[i].first);
            }
        }

        if (numFsms % 2 == 1) {
            fsms[numFsms / 2].first = std::move(fsms[numFsms - 1].first);
        }

        numFsms = (numFsms + 1) / 2;
    }

    fsms[0].first.Complement();

    return TEnumerableScanner(fsms[0].first);
}


} // anonymous namespace


TPreparedRegexCondition::TPreparedRegexCondition(const TVector<TRegexCondition>& conditions) {
    TVector<std::pair<NPire::TFsm, bool>> fsms;
    fsms.reserve(conditions.size());

    for (const auto& condition : conditions) {
        std::visit([this, &fsms, &condition] <typename T> (const T& value) {
            if constexpr (std::is_same_v<T, TRegexMatcherEntry>) {
                NPire::TLexer lexer(value.Expr);
                value.SetUpLexer(&lexer);

                fsms.push_back({lexer.Parse(), condition.ShouldMatch});
            } else if constexpr (std::is_same_v<T, TCbbGroupId>) {
                GroupConditions.push_back({
                    .GroupId = value,
                    .ShouldMatch = condition.ShouldMatch
                });
            }
        }, condition.Value);
    }

    Scanner = CompileFsms(fsms);
}


TPreparedFactorCondition::TPreparedFactorCondition(const TFactorCondition& condition)
    : FactorIndex(TRawCacherFactors::GetIndexByName(condition.FactorName))
    , Comparator(condition.Comparator)
{}


TPreparedRule::TPreparedRule(TRule rule)
    : TRule(std::move(rule))
    , PreparedDoc(Doc)
    , PreparedCgiString(CgiString)
    , PreparedIdentType(IdentType)
    , PreparedArrivalTime(ArrivalTime)
    , PreparedServiceType(ServiceType)
    , PreparedRequest(Request)
    , PreparedHodor(Hodor)
    , PreparedHodorHash(HodorHash)
    , PreparedJwsInfo(JwsInfo)
    , PreparedYandexTrustInfo(YandexTrustInfo)
    , PreparedCountryId(CountryId)
{
    PreparedHeaders.reserve(Headers.size());

    for (const auto& [headerName, conditions] : Headers) {
        PreparedHeaders[headerName] = TPreparedRegexCondition(conditions);
    }

    PreparedCsHeaders.reserve(CsHeaders.size());

    for (const auto& [headerName, conditions] : CsHeaders) {
        PreparedCsHeaders[headerName] = TPreparedRegexCondition(conditions);
    }

    for (const auto& factorCondition : Factors) {
        PreparedFactors.push_back(TPreparedFactorCondition(factorCondition));
    }

    SortBy(PreparedFactors, [] (const auto& factor) {
        return factor.FactorIndex;
    });
}

TPreparedRule TPreparedRule::Parse(const TString& s) {
    return TPreparedRule(NMatchRequest::ParseRule(s));
}

TVector<TPreparedRule> TPreparedRule::ParseList(
    const TVector<TString>& rules,
    TVector<TString>* errors
) {
    TVector<TPreparedRule> parsedRules;
    parsedRules.reserve(rules.size());

    size_t line = 1;

    for (const auto& rule : rules) {
        TPreparedRule parsedRule;

        try {
            parsedRule = TPreparedRule::Parse(rule);
        } catch (const std::exception& exc) {
            if (errors) {
                errors->push_back(TStringBuilder() << "rule " << line << ": " << exc.what());
            }
        }

        parsedRules.push_back(std::move(parsedRule));
        ++line;
    }

    return parsedRules;
}


} // namespace NAntiRobot
