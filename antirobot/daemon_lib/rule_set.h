#pragma once

#include "addr_list.h"
#include "cacher_factors.h"
#include "cbb_id.h"
#include "cbb_iplist_manager.h"
#include "cbb_relist_manager.h"
#include "exp_bin.h"
#include "prepared_rule.h"
#include "request_params.h"
#include "rule.h"

#include <antirobot/lib/enumerable_scanner.h>

#include <library/cpp/iterator/functools.h>

#include <library/cpp/regex/pire/pire.h>

#include <util/digest/sequence.h>
#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/xrange.h>

#include <array>
#include <variant>


namespace NAntiRobot {


using TRuleIndex = ui32;


class TStringByRegexMatcher;

template <typename EEnum>
class TEnumByRegexMatcher;

class TRuleSet;


struct TStringRegexListOffset {
    using TMatcher = TStringByRegexMatcher;

    TPreparedRegexCondition TPreparedRule::*Offset = nullptr;

    TStringRegexListOffset(
        TPreparedRegexCondition TPreparedRule::*offset
    )
        : Offset(offset)
    {}

    const TPreparedRegexCondition& Access(const TPreparedRule& rule) const {
        return rule.*Offset;
    }
};


template <typename EEnum>
struct TEnumRegexListOffset : public TStringRegexListOffset {
    using TMatcher = TEnumByRegexMatcher<EEnum>;

    explicit TEnumRegexListOffset(
        TPreparedRegexCondition TPreparedRule::*offset
    )
        : TStringRegexListOffset{offset}
    {}
};


struct THeaderRegexListOffset {
    using TMatcher = TStringByRegexMatcher;

    THashMap<TString, TPreparedRegexCondition> TPreparedRule::*Offset = nullptr;
    TString Key;

    THeaderRegexListOffset(
        THashMap<TString, TPreparedRegexCondition> TPreparedRule::*offset,
        TString key
    )
        : Offset(offset)
        , Key(std::move(key))
    {}

    const TPreparedRegexCondition& Access(const TPreparedRule& rule) const {
        static const TPreparedRegexCondition empty;

        const auto& map = rule.*Offset;
        if (const auto vec = map.FindPtr(Key)) {
            return *vec;
        }

        return empty;
    }
};


using TRuleParamOffset = std::variant<
    TStringRegexListOffset,
    TEnumRegexListOffset<EHostType>,
    TEnumRegexListOffset<EJwsState>,
    TEnumRegexListOffset<EYandexTrustState>,
    THeaderRegexListOffset
>;


using TRequestParamValue = std::variant<TStringBuf, EHostType, EJwsState, EYandexTrustState>;


using TPatternCache = THashMap<
    TVector<std::tuple<bool, bool, TString>>,
    TEnumerableScanner,
    TRangeHash<>
>;


class TStringByRegexMatcher {
public:
    using TState = NPire::TNonrelocScanner::State;
    using TValue = TStringBuf;

public:
    TStringByRegexMatcher() = default;

    explicit TStringByRegexMatcher(
        const TVector<TPreparedRule>& rules,
        const TVector<TRuleIndex>& ruleIndices,
        const TRuleParamOffset& regexListOffset
    );

    TVector<std::pair<size_t, TVector<TRuleIndex>>> EnumerateStates() const;

    void MatchStates(TStringBuf s, TVector<size_t>* out) const;
    void MatchRules(TStringBuf s, TVector<TRuleIndex>* out) const;

private:
    struct TEntry {
        TVector<TRuleIndex> RuleIndices;
        THashMap<size_t, size_t> Remap;
        TEnumerableScanner Scanner;
    };

    TVector<TEntry> Entries;
};


template <typename EEnum>
class TEnumByRegexMatcher {
public:
    using TValue = EEnum;

public:
    TEnumByRegexMatcher() = default;

    explicit TEnumByRegexMatcher(
        const TVector<TPreparedRule>& rules,
        const TVector<TRuleIndex>& ruleIndices,
        TStringRegexListOffset regexListOffset
    );

    TVector<std::pair<size_t, TVector<TRuleIndex>>> EnumerateStates() const;

    void MatchStates(EEnum value, TVector<size_t>* out) const;
    void MatchRules(EEnum value, TVector<TRuleIndex>* out) const;

private:
    std::array<TVector<TRuleIndex>, static_cast<size_t>(EEnum::Count)> EnumToRuleIndices;
};


struct TFinalChecker {
    struct TGroupEntry {
        size_t Depth = 0;
        TCbbGroupId Id{};
        bool ShouldMatch = false;
    };

    TCbbRuleKey Key;
    ui16 CheckMask = 0;
    ui16 TargetMask = 0;
    TIpInterval IpInterval = TIpInterval::MakeEmpty();
    EExpBin ExpBin = EExpBin::Count;
    TConstantComparator<TInexactFloat3> CookieAge;
    TVector<TConstantComparator<unsigned>> CurrentTimestamp;
    float RandomThreshold = 1;
    TRefreshableAddrSet Addrs;
    TVector<TPreparedFactorCondition> Factors;
    TVector<std::pair<size_t, size_t>> HeaderDepthsCounts;
    TVector<TGroupEntry> GroupEntries;
    bool Whitelisting = true;

    TFinalChecker() = default;

    explicit TFinalChecker(
        TCbbGroupId groupId,
        const TPreparedRule& rule,
        THashMap<TCbbGroupId, TRefreshableAddrSet>* idToAddrList,
        TCbbIpListManager* ipListManager = nullptr
    );
};


struct TMatchEntry {
    TCbbRuleKey Key;
    bool Whitelisting = false;
    EExpBin ExpBin = EExpBin::Count;

    TBinnedCbbRuleKey ToBinnedKey() const {
        return TBinnedCbbRuleKey(Key, ExpBin);
    }
};


struct TMatchContext {
    const TRuleSet* RuleSet = nullptr;
    TVector<TRuleIndex> TmpIndices;
    TVector<size_t> TmpStates;
    THashMap<std::pair<size_t, TCbbGroupId>, bool> DepthIdToPatternListMatches;
    const TRequest* Req = nullptr;
    const TRawCacherFactors* CacherFactors = nullptr;
    ui16 FlagMask = 0;
    TVector<TRequestParamValue> DepthToParam;
    bool AddrWhitelisted = false;
    TVector<size_t> HeaderCounts;
    TVector<TMatchEntry> Output;

    bool FinalCheck(TRuleIndex ruleIndex);

    void Push(TRuleIndex ruleIndex);
};


class TRuleSetTreeNode {
public:
    TRuleSetTreeNode() = default;

    explicit TRuleSetTreeNode(
        size_t depth,
        const TVector<TPreparedRule>& rules,
        const TVector<TRuleIndex>& ruleIndices,
        const TVector<TRuleParamOffset>& depthToRuleParamOffset
    );

    void Match(TMatchContext* ctx) const;

private:
    using TMatcher = std::variant<
        TStringByRegexMatcher,
        TEnumByRegexMatcher<EHostType>,
        TEnumByRegexMatcher<EJwsState>,
        TEnumByRegexMatcher<EYandexTrustState>
    >;

    size_t Depth = 0;
    TMatcher Matcher;
    THashMap<TStringByRegexMatcher::TState, TRuleSetTreeNode> StateToChild;
    THolder<TRuleSetTreeNode> Epsilon;
    TVector<TRuleIndex> InactiveLeafRuleIndices;
};


class TRuleSet {
    friend TMatchContext;

public:
    TRuleSet() = default;

    explicit TRuleSet(
        const TVector<std::pair<TCbbGroupId, TVector<TPreparedRule>>>& idsGroups,
        TRefreshableAddrSet nonBlockAddrs = nullptr,
        TCbbIpListManager* maybeIpListManager = nullptr,
        TCbbReListManager* maybeReListManager = nullptr
    );

    TVector<TMatchEntry> Match(
        const TRequest& req,
        const TRawCacherFactors* cacherFactors = nullptr
    ) const;

    bool Matches(const TRequest& req, const TRawCacherFactors* cacherFactors = nullptr) const {
        return !Match(req, cacherFactors).empty();
    }

    size_t NumRoots() const {
        return Roots.size();
    }

    void RemoveGroups() const;

private:
    struct TRootData {
        TVector<size_t> Mask;
        TRuleSetTreeNode Root;
    };

    THashMap<TCbbGroupId, TRefreshableAddrSet> IdToAddrList;
    THashMap<TCbbGroupId, TRegexMatcherAccessorPtr> IdToPatternList;
    TRefreshableAddrSet NonBlockAddrs;
    TCbbIpListManager* IpListManager = nullptr;
    TCbbReListManager* ReListManager = nullptr;
    TVector<TFinalChecker> FinalCheckers;
    TVector<TRuleParamOffset> DepthToOffset;
    TVector<TString> HeaderNames;
    TVector<TString> CsHeaderNames;
    TVector<TRootData> Roots;
};


} // namespace NAntiRobot
