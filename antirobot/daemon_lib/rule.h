#pragma once

#include "cbb_id.h"
#include "exp_bin.h"

#include <antirobot/lib/comparator.h>
#include <antirobot/lib/regex_matcher.h>

#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>

#include <variant>


namespace NAntiRobot {


class TParseRuleError : public yexception {};


struct TRegexCondition {
    std::variant<TRegexMatcherEntry, TCbbGroupId> Value;
    bool ShouldMatch = true;

    TRegexCondition() = default;

    TRegexCondition(TString expr, bool caseSensitive, bool matchEquality)
        : Value(TRegexMatcherEntry(std::move(expr), caseSensitive))
        , ShouldMatch(matchEquality)
    {}

    TMaybe<TCbbGroupId> GetCbbGroupId() const {
        if (const auto ret = std::get_if<TCbbGroupId>(&Value)) {
            return *ret;
        }

        return Nothing();
    }

    auto operator<=>(const TRegexCondition&) const = default;
};


struct THasHeaderCondition {
    TString HeaderName;
    bool Present = true;

    THasHeaderCondition() = default;

    THasHeaderCondition(TString headerName, bool present)
        : HeaderName(std::move(headerName))
        , Present(present)
    {}

    auto operator<=>(const THasHeaderCondition&) const = default;
};


struct TNumHeaderCondition {
    TString HeaderName;
    ui32 Count = 0;

    TNumHeaderCondition() = default;

    TNumHeaderCondition(TString headerName, ui32 count)
        : HeaderName(std::move(headerName))
        , Count(count)
    {}

    auto operator<=>(const TNumHeaderCondition&) const = default;
};


struct TFactorCondition {
    TString FactorName;
    TConstantComparator<TInexactFloat3> Comparator;

    TFactorCondition() = default;

    TFactorCondition(TString factorName, TConstantComparator<TInexactFloat3> comparator)
        : FactorName(std::move(factorName))
        , Comparator(comparator)
    {}

    auto operator<=>(const TFactorCondition&) const = default;
};


struct TRule {
    TCbbRuleId RuleId{0};
    bool Nonblock = false;
    bool Enabled = true;
    bool IsTor = false;
    bool IsProxy = false;
    bool IsVpn = false;
    bool IsHosting = false;
    bool IsMobile = false;
    bool IsMikrotik = false;
    bool IsSquid = false;
    bool IsDdoser = false;

    TVector<TRegexCondition> Doc;
    TVector<TRegexCondition> CgiString;
    TVector<TRegexCondition> IdentType;
    TVector<TRegexCondition> ArrivalTime;
    TVector<TRegexCondition> ServiceType;
    TVector<TRegexCondition> Request;
    TVector<TRegexCondition> Hodor;
    TVector<TRegexCondition> HodorHash;
    TVector<TRegexCondition> JwsInfo;
    TVector<TRegexCondition> YandexTrustInfo;
    TVector<TRegexCondition> CountryId;

    // Case-insensitive headers.
    THashMap<TString, TVector<TRegexCondition>> Headers;
    TVector<THasHeaderCondition> HasHeaders;
    TVector<TNumHeaderCondition> NumHeaders;

    // Case-sensitive headers.
    THashMap<TString, TVector<TRegexCondition>> CsHeaders;
    TVector<THasHeaderCondition> HasCsHeaders;
    TVector<TNumHeaderCondition> NumCsHeaders;

    TIpInterval IpInterval = TIpInterval::MakeEmpty();
    TMaybe<TCbbGroupId> CbbGroup;
    TMaybe<bool> IsWhitelist; // from geobase + Whitetlist
    TMaybe<bool> Degradation; // degradation spravka
    TMaybe<bool> PanicMode;
    TMaybe<bool> InRobotSet;
    TMaybe<bool> MayBan;
    TMaybe<float> RandomThreshold;
    TMaybe<EExpBin> ExpBin;
    TMaybe<bool> ValidAutoRuTamper;
    TConstantComparator<TInexactFloat3> CookieAge;
    TVector<TConstantComparator<unsigned>> CurrentTimestamp;
    TVector<TFactorCondition> Factors;

    bool Empty() const;
};


} // namespace NAntiRobot
