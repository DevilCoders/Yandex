#pragma once

#include <antirobot/daemon_lib/addr_list.h>
#include <antirobot/daemon_lib/cbb_id.h>
#include <antirobot/daemon_lib/cbb_relist_manager.h>
#include <antirobot/daemon_lib/request_params.h>
#include <antirobot/daemon_lib/rule.h>

#include <antirobot/lib/ar_utils.h>
#include <antirobot/lib/ip_list.h>
#include <antirobot/lib/regex_matcher.h>

#include <library/cpp/iterator/filtering.h>
#include <library/cpp/iterator/mapped.h>

#include <library/cpp/regex/pire/pire.h>

#include <library/cpp/threading/rcu/rcu_accessor.h>

#include <util/generic/maybe.h>
#include <util/generic/variant.h>
#include <util/generic/yexception.h>
#include <util/system/rwlock.h>

#include <array>
#include <functional>


namespace NAntiRobot {
    class TAlarmer;
    class ICbbIO;
    class TCbbIpListManager;


    namespace NOldMatchRequest {
        struct TMultiRegexMatcher {
            TRegexMatcherAccessorPtr AccessorPtr;

            TMultiRegexMatcher() = default;

            explicit TMultiRegexMatcher(TRegexMatcherAccessorPtr accessorPtr)
                : AccessorPtr(std::move(accessorPtr))
            {}

            bool Matches(TStringBuf s) const {
                const auto matcher = AccessorPtr->Get();
                return matcher->Defined() && (*matcher)->Matches(s);
            }
        };

        struct TRegex {
            TMaybe<TRegexConjunction> MatcherTrue;
            TMaybe<TRegexMatcher> MatcherFalse;
            TVector<TMultiRegexMatcher> MultiMatchersTrue;
            TVector<TMultiRegexMatcher> MultiMatchersFalse;

            TRegex() = default;

            explicit TRegex(
                const TVector<TRegexCondition>& src,
                TAlarmer* maybeAlarmer = nullptr,
                ICbbIO* maybeCbb = nullptr,
                TCbbReListManager* maybeCbbReListManager = nullptr
            );

            bool Matches(TStringBuf s) const {
                if (
                    !MatcherTrue &&  MultiMatchersTrue.empty() &&
                    !MatcherFalse && MultiMatchersFalse.empty()
                ) {
                    return false;
                }
                if (MatcherTrue && !MatcherTrue->Matches(s)) {
                    return false;
                }
                if (MatcherFalse && MatcherFalse->Matches(s)) {
                    return false;
                }
                for (const auto& matcher : MultiMatchersTrue) {
                    if (!matcher.Matches(s)) {
                        return false;
                    }
                }
                for (const auto& matcher : MultiMatchersFalse) {
                    if (matcher.Matches(s)) {
                        return false;
                    }
                }
                return true;
            }
        };

        struct THeaderRegex {
            TString HeaderName;
            TRegex Matcher;

            THeaderRegex() = default;

            THeaderRegex(TString headerName, TRegex matcher)
                : HeaderName(std::move(headerName))
                , Matcher(std::move(matcher))
            {}

            bool Matches(const TRequest& req) const {
                return req.Headers.Has(HeaderName) &&
                    Matcher.Matches(req.Headers.Get(HeaderName));
            }

            bool CSMatches(const TRequest& req) const {
                const auto range = req.CSHeaders.equal_range(HeaderName);
                if (range.first == req.CSHeaders.end()) {
                    return false;
                }
                return Matcher.Matches(range.first->second);
            }
        };

        class TRulePreparationError : public yexception {};

        class TPreparedMatchRule : public TRule {
        private:
            TRegex DocMatcher;
            TRegex CgiStringMatcher;
            TRegex IdentTypeMatcher;
            TRegex ArrivalTimeMatcher;
            TRegex ServiceTypeMatcher;
            TRegex CountryIdMatcher;
            TRegex RequestMatcher;
            TRegex HodorMatcher;
            TRegex HodorHashMatcher;
            TRegex JwsInfoMatcher;
            TRegex YandexTrustInfoMatcher;
            TVector<THeaderRegex> RegexHeaders;
            TVector<THeaderRegex> RegexCSHeaders;
            TRefreshableAddrSet Addrs;
            TRefreshableAddrSet NonBlockAddrs;
            TVector<bool(*)(const TPreparedMatchRule* self, const TRequest& req)> NonEmptyCheckers;

        public:
            explicit TPreparedMatchRule(
                const TRule& rule,
                const TRefreshableAddrSet* maybeNonBlockAddrs = nullptr,
                TAlarmer* maybeAlarmer = nullptr,
                ICbbIO* maybeCbb = nullptr,
                TCbbIpListManager* maybeCbbIpListManager = nullptr,
                TCbbReListManager* maybeCbbReListManager = nullptr
            );

            bool Match(const TRequest& req) const;
        };

        struct TParseRuleError : public yexception {
        };

        class TRuleSet {
        public:
            TRuleSet() = default;

            explicit TRuleSet(TVector<TPreparedMatchRule> rules)
                : Rules(std::move(rules))
            {}

            void AddRule(const TPreparedMatchRule& rule);
            bool IsMatched(const TRequest& request) const;

            auto Match(const TRequest& request) const {
                auto matched = MakeFilteringRange(Rules, [&request] (const auto& rule) {
                    return rule.Match(request);
                });

                return MakeMappedRange(std::move(matched), [] (const auto& rule) {
                    return rule.RuleId;
                });
            }

            bool IsEmpty() const {
                return Rules.empty();
            }

            const TVector<TPreparedMatchRule>& GetRules() const {
                return Rules;
            }

        private:
            TVector<TPreparedMatchRule> Rules;
        };

        TVector<TPreparedMatchRule> ParseRules(
            const TVector<TString>& rulesLines,
            TVector<TString>* errors,
            TVector<TString>* maybePreparationErrors = nullptr,
            const TRefreshableAddrSet* maybeNonBlockAddrs = nullptr,
            TAlarmer* maybeAlarmer = nullptr,
            ICbbIO* maybeCbb = nullptr,
            TCbbIpListManager* maybeCbbIpListManager = nullptr,
            TCbbReListManager* maybeCbbReListManager = nullptr
        );
    } // namespace NOldMatchRequest
} // namespace NAntiRobot

