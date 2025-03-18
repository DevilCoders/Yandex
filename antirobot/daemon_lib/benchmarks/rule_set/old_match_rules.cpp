#include "old_match_rules.h"

#include <antirobot/daemon_lib/cbb_iplist_manager.h>
#include <antirobot/daemon_lib/eventlog_err.h>
#include <antirobot/daemon_lib/match_rule_lexer.h>
#include <antirobot/daemon_lib/match_rule_parser.h>

#include <util/generic/algorithm.h>
#include <util/generic/variant.h>
#include <util/generic/vector.h>
#include <util/generic/xrange.h>
#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/string/join.h>
#include <util/string/util.h>

#include <functional>
#include <type_traits>

namespace NAntiRobot {
    namespace NOldMatchRequest {
        TRegex::TRegex(
            const TVector<TRegexCondition>& src,
            TAlarmer* maybeAlarmer,
            ICbbIO* maybeCbb,
            TCbbReListManager* maybeCbbReListManager
        ) {
            for (const auto& res : src) {
                std::visit([this, matchEquality = res.ShouldMatch, maybeAlarmer, maybeCbb, maybeCbbReListManager] (const auto& concreteValue) {
                    using TConcreteType = std::remove_cvref_t<decltype(concreteValue)>;

                    if constexpr (std::is_same_v<TConcreteType, TRegexMatcherEntry>) {
                        if (matchEquality) {
                            if (!MatcherTrue) {
                                MatcherTrue = TRegexConjunction(concreteValue);
                            } else {
                                MatcherTrue->AddScanner(concreteValue);
                            }
                        } else {
                            if (!MatcherFalse) {
                                MatcherFalse = TRegexMatcher(concreteValue);
                            } else {
                                MatcherFalse->AddScanner(concreteValue);
                            }
                        }
                    } else if constexpr (std::is_same_v<TConcreteType, TCbbGroupId>) {
                        Y_ENSURE_EX(
                            maybeAlarmer && maybeCbb && maybeCbbReListManager,
                            TRulePreparationError() << "group regexps require access to CBB"
                        );

                        if (matchEquality) {
                            MultiMatchersTrue.push_back(TMultiRegexMatcher(maybeCbbReListManager->Add(concreteValue)));
                        } else {
                            MultiMatchersFalse.push_back(TMultiRegexMatcher(maybeCbbReListManager->Add(concreteValue)));
                        }
                    } else {
                        static_assert(TDependentFalse<TConcreteType>);
                    }
                },
                res.Value);
            }
            if (MatcherTrue) {
                MatcherTrue->Prepare();
            }
            if (MatcherFalse){
                MatcherFalse->Prepare();
            }
        }

        TPreparedMatchRule::TPreparedMatchRule(
            const TRule& rule,
            const TRefreshableAddrSet* maybeNonBlockAddrs,
            TAlarmer* maybeAlarmer,
            ICbbIO* maybeCbb,
            TCbbIpListManager* maybeCbbIpListManager,
            TCbbReListManager* maybeCbbReListManager
        )
            : TRule(rule)
        {
#define HANDLE_PARAM(name, f) \
            if (!rule.name.empty()) { \
                name##Matcher = TRegex(rule.name, maybeAlarmer, maybeCbb, maybeCbbReListManager); \
                NonEmptyCheckers.push_back([] (const TPreparedMatchRule* self, const TRequest& req) { \
                    return self->name##Matcher.Matches(f); \
                }); \
            }

            HANDLE_PARAM(Doc, req.Doc);
            HANDLE_PARAM(CgiString, req.Cgi);
            HANDLE_PARAM(IdentType, ToString(req.Uid));
            HANDLE_PARAM(ArrivalTime, ToString(req.ArrivalTime.MicroSeconds()));
            HANDLE_PARAM(ServiceType, ToString(req.HostType));
            HANDLE_PARAM(CountryId, ToString(req.UserAddr.CountryId()));
            HANDLE_PARAM(Request, req.Request);
            HANDLE_PARAM(Hodor, ToString(req.Hodor));
            HANDLE_PARAM(HodorHash, ToString(req.HodorHash));
            HANDLE_PARAM(JwsInfo, ToString(req.JwsState));
            HANDLE_PARAM(YandexTrustInfo, ToString(req.YandexTrustState));

#undef HANDLE_PARAM

            if (rule.IsTor) {
                NonEmptyCheckers.push_back([](const TPreparedMatchRule*, const TRequest& req) {
                    return req.UserAddr.IsTor();
                });
            }

            if (rule.IsProxy) {
                NonEmptyCheckers.push_back([](const TPreparedMatchRule*, const TRequest& req) {
                    return req.UserAddr.IsProxy();
                });
            }

            if (rule.IsVpn) {
                NonEmptyCheckers.push_back([](const TPreparedMatchRule*, const TRequest& req) {
                    return req.UserAddr.IsVpn();
                });
            }

            if (rule.IsHosting) {
                NonEmptyCheckers.push_back([](const TPreparedMatchRule*, const TRequest& req) {
                    return req.UserAddr.IsHosting();
                });
            }

            if (rule.IsMikrotik) {
                NonEmptyCheckers.push_back([](const TPreparedMatchRule*, const TRequest& req) {
                    return req.UserAddr.IsMikrotik();
                });
            }

            if (rule.IsSquid) {
                NonEmptyCheckers.push_back([](const TPreparedMatchRule*, const TRequest& req) {
                    return req.UserAddr.IsSquid();
                });
            }

            if (rule.IsDdoser) {
                NonEmptyCheckers.push_back([](const TPreparedMatchRule*, const TRequest& req) {
                    return req.UserAddr.IsDdoser();
                });
            }

            if (rule.IsMobile) {
                NonEmptyCheckers.push_back([](const TPreparedMatchRule* self, const TRequest& req) {
                    Y_UNUSED(self);
                    return req.UserAddr.IsMobile();
                });
            }

            if (rule.IsWhitelist.Defined()) {
                NonEmptyCheckers.push_back([](const TPreparedMatchRule* self, const TRequest& req) {
                    const bool inWhitelist = req.UserAddr.IsWhitelisted() || req.UserAddr.IsYandexNet();
                    return *self->IsWhitelist ? inWhitelist : !inWhitelist;
                });
            }

            if (rule.Degradation.Defined()) {
                NonEmptyCheckers.push_back([](const TPreparedMatchRule* self, const TRequest& req) {
                    return *self->Degradation ? req.Degradation : !req.Degradation;
                });
            }

            if (rule.PanicMode.Defined()) {
                NonEmptyCheckers.push_back([](const TPreparedMatchRule* self, const TRequest& req) {
                    return *self->PanicMode ? req.CbbPanicMode : !req.CbbPanicMode;
                });
            }

            if (rule.InRobotSet.Defined()) {
                NonEmptyCheckers.push_back([](const TPreparedMatchRule* self, const TRequest& req) {
                    return *self->InRobotSet ? req.InRobotSet : !req.InRobotSet;
                });
            }

            if (!rule.IpInterval.IsEmpty()) {
                NonEmptyCheckers.push_back([](const TPreparedMatchRule* self, const TRequest& req) {
                    return self->IpInterval.HasAddr(req.UserAddr);
                });
            }

            if (rule.CbbGroup.Defined()) {
                Y_ENSURE_EX(
                    maybeCbbIpListManager,
                    TRulePreparationError() << "ip_from requires access to CBB"
                );

                Addrs = maybeCbbIpListManager->Add({*rule.CbbGroup});

                if (maybeNonBlockAddrs) {
                    NonBlockAddrs = *maybeNonBlockAddrs;
                    NonEmptyCheckers.push_back([] (const TPreparedMatchRule* self, const TRequest& req) {
                        return
                            !self->NonBlockAddrs->Get()->ContainsActual(req.UserAddr) &&
                            self->Addrs->Get()->ContainsActual(req.UserAddr);
                    });
                } else {
                    NonEmptyCheckers.push_back([] (const TPreparedMatchRule* self, const TRequest& req) {
                        return self->Addrs->Get()->ContainsActual(req.UserAddr);
                    });
                }
            }

            // case insensitive Headers
            if (!rule.Headers.empty()) {
                for (const auto& [header, values] : Headers) {
                    auto regex = TRegex(
                        values,
                        maybeAlarmer,
                        maybeCbb,
                        maybeCbbReListManager
                    );
                    RegexHeaders.emplace_back(header, std::move(regex));
                }

                if (!RegexHeaders.empty()) {
                    NonEmptyCheckers.push_back([] (const TPreparedMatchRule* self, const TRequest& req) {
                        for (const auto& headerRegex : self->RegexHeaders) {
                            if (!headerRegex.Matches(req)) {
                                return false;
                            }
                        }

                        return true;
                    });
                }
            }

            if (!rule.HasHeaders.empty()) {
                NonEmptyCheckers.push_back([] (const TPreparedMatchRule* self, const TRequest& req) {
                    for (const auto& hasHeader : self->HasHeaders) {
                        if (req.Headers.Has(hasHeader.HeaderName) != hasHeader.Present) {
                            return false;
                        }
                    }

                    return true;
                });
            }

            if (!rule.NumHeaders.empty()) {
                NonEmptyCheckers.push_back([] (const TPreparedMatchRule* self, const TRequest& req) {
                    for (const auto& numHeader : self->NumHeaders) {
                        if (req.Headers.NumOfValues(numHeader.HeaderName) != numHeader.Count) {
                            return false;
                        }
                    }

                    return true;
                });
            }

            // case_sensitive_ headers
            if (!rule.CsHeaders.empty()) {
                for (const auto& [header, values] : CsHeaders) {
                    auto regex = TRegex(
                        values,
                        maybeAlarmer,
                        maybeCbb,
                        maybeCbbReListManager
                    );
                    RegexCSHeaders.emplace_back(header, std::move(regex));
                }

                if (!RegexCSHeaders.empty()) {
                    NonEmptyCheckers.push_back([] (const TPreparedMatchRule* self, const TRequest& req) {
                        for (const auto& headerRegex : self->RegexCSHeaders) {
                            if (!headerRegex.CSMatches(req)) {
                                return false;
                            }
                        }

                        return true;
                    });
                }
            }

            if (!rule.HasCsHeaders.empty()) {
                NonEmptyCheckers.push_back([] (const TPreparedMatchRule* self, const TRequest& req) {
                    for (const auto& hasHeader : self->HasCsHeaders) {
                        if (req.CSHeaders.contains(hasHeader.HeaderName) != hasHeader.Present) {
                            return false;
                        }
                    }

                    return true;
                });
            }

            if (!rule.NumCsHeaders.empty()) {
                NonEmptyCheckers.push_back([] (const TPreparedMatchRule* self, const TRequest& req) {
                    for (const auto& numHeader : self->NumCsHeaders) {
                        if (req.CSHeaders.count(numHeader.HeaderName) != numHeader.Count) {
                            return false;
                        }
                    }

                    return true;
                });
            }

            if (rule.RandomThreshold.Defined()) {
                NonEmptyCheckers.push_back([] (const TPreparedMatchRule* self, const TRequest& req) {
                    Y_UNUSED(req);
                    return RandomNumber<float>() < self->RandomThreshold;
                });
            }

            if (rule.MayBan.Defined()) {
                NonEmptyCheckers.push_back([](const TPreparedMatchRule* self, const TRequest& req) {
                    return *self->MayBan ? req.CbbMayBan() : !req.CbbMayBan();
                });
            }

            if (rule.ExpBin.Defined()) {
                NonEmptyCheckers.push_back([](const TPreparedMatchRule* self, const TRequest& req) {
                    return *self->ExpBin == req.ExperimentBin();
                });
            }

            if (rule.ValidAutoRuTamper.Defined()) {
                NonEmptyCheckers.push_back([](const TPreparedMatchRule* self, const TRequest& req) {
                    return *self->ValidAutoRuTamper == req.HasValidAutoRuTamper;
                });
            }

            if (!rule.CookieAge.IsNop()) {
                NonEmptyCheckers.push_back([](const TPreparedMatchRule* self, const TRequest& req) {
                    return self->CookieAge.Compare(req.CookieAge);
                });
            }

            if (!rule.CurrentTimestamp.empty()) {
                NonEmptyCheckers.push_back([](const TPreparedMatchRule* self, const TRequest& req) {
                    for (const auto& timestamp : self->CurrentTimestamp) {
                        if (!timestamp.Compare(req.CurrentTimestamp)) {
                            return false;
                        }
                    }
                    return true;
                });
            }
        }

        bool TPreparedMatchRule::Match(const TRequest& req) const {
            return !NonEmptyCheckers.empty() && AllOf(NonEmptyCheckers, [&req,this](auto checker) { return checker(this, req); });
        }

        void TRuleSet::AddRule(const TPreparedMatchRule& rule) {
            Rules.push_back(rule);
        }

        bool TRuleSet::IsMatched(const TRequest& req) const {
            return AnyOf(Rules, [&req] (const auto& rule) {
                return rule.Match(req);
            });
        }

        TVector<TPreparedMatchRule> ParseRules(
            const TVector<TString>& rulesLines,
            TVector<TString>* syntaxErrors,
            TVector<TString>* maybePreparationErrors,
            const TRefreshableAddrSet* maybeNonBlockAddrs,
            TAlarmer* maybeAlarmer,
            ICbbIO* maybeCbb,
            TCbbIpListManager* maybeCbbIpListManager,
            TCbbReListManager* maybeCbbReListManager
        ) {
            TVector<TPreparedMatchRule> retval;

            syntaxErrors->clear();

            if (maybePreparationErrors) {
                maybePreparationErrors->clear();
            }

            for (auto i : xrange(rulesLines.size())) {
                try {
                    retval.push_back(TPreparedMatchRule(
                        NMatchRequest::ParseRule(rulesLines[i]),
                        maybeNonBlockAddrs,
                        maybeAlarmer,
                        maybeCbb,
                        maybeCbbIpListManager,
                        maybeCbbReListManager
                    ));
                } catch (const TParseRuleError& ex) {
                    syntaxErrors->push_back(
                        TStringBuilder() << "Error on line " << i + 1 << ": " << ex.what()
                    );
                } catch (const TRulePreparationError& ex) {
                    if (maybePreparationErrors) {
                        maybePreparationErrors->push_back(
                            TStringBuilder() << "Error on line " << i + 1 << ": " << ex.what()
                        );
                    }
                }
            }

            return retval;
        }
    }
}
