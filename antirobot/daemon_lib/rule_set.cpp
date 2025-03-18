#include "rule_set.h"

#include "cbb_iplist_manager.h"
#include "util/string/join.h"


namespace NAntiRobot {


namespace {


const auto STATIC_OFFSETS = std::to_array<TRuleParamOffset>({
    TEnumRegexListOffset<EHostType>(&TPreparedRule::PreparedServiceType),
    TEnumRegexListOffset<EJwsState>(&TPreparedRule::PreparedJwsInfo),
    TEnumRegexListOffset<EYandexTrustState>(&TPreparedRule::PreparedYandexTrustInfo),
    TStringRegexListOffset(&TPreparedRule::PreparedIdentType),
    TStringRegexListOffset(&TPreparedRule::PreparedDoc),
    TStringRegexListOffset(&TPreparedRule::PreparedCgiString),
    TStringRegexListOffset(&TPreparedRule::PreparedArrivalTime),
    TStringRegexListOffset(&TPreparedRule::PreparedRequest),
    TStringRegexListOffset(&TPreparedRule::PreparedHodor),
    TStringRegexListOffset(&TPreparedRule::PreparedHodorHash),
    TStringRegexListOffset(&TPreparedRule::PreparedCountryId)
});


void FillCheckerMasks(const TRule& rule, ui16& checkMask, ui16& targetMask) {
    static const std::array boolOffsets = {
        &TRule::IsTor,
        &TRule::IsProxy,
        &TRule::IsVpn,
        &TRule::IsHosting,
        &TRule::IsMobile,
        &TRule::IsMikrotik,
        &TRule::IsSquid,
        &TRule::IsDdoser,
    };

    static const std::array maybeOffsets = {
        &TRule::IsWhitelist,
        &TRule::Degradation,
        &TRule::PanicMode,
        &TRule::InRobotSet,
        &TRule::MayBan,
        &TRule::ValidAutoRuTamper,
    };

    static_assert(boolOffsets.size() + maybeOffsets.size() <= sizeof(checkMask) * 8);

    size_t i = 0;

    for (const auto offset : boolOffsets) {
        const auto flag = static_cast<ui16>(rule.*offset) << i;
        checkMask |= flag;
        targetMask |= flag;
        ++i;
    }

    for (const auto offset : maybeOffsets) {
        const auto flag = rule.*offset;
        if (flag.Defined()) {
            checkMask |= 1 << i;
            targetMask |= static_cast<ui16>(*flag) << i;
        }

        ++i;
    }
}


ui16 MakeFlagMask(const TRequest& req) {
    ui16 flagMask = 0;
    size_t i = 0;

    for (const auto flag : {
        req.UserAddr.IsTor(),
        req.UserAddr.IsProxy(),
        req.UserAddr.IsVpn(),
        req.UserAddr.IsHosting(),
        req.UserAddr.IsMobile(),
        req.UserAddr.IsMikrotik(),
        req.UserAddr.IsSquid(),
        req.UserAddr.IsDdoser(),
        req.UserAddr.IsWhitelisted() || req.UserAddr.IsYandexNet(),
        req.Degradation,
        req.CbbPanicMode,
        req.InRobotSet,
        req.CbbMayBan(),
        req.HasValidAutoRuTamper,
    }) {
        flagMask |= static_cast<ui16>(flag) << i;
        ++i;
    }

    return flagMask;
}


} // anonymous namespace


TStringByRegexMatcher::TStringByRegexMatcher(
    const TVector<TPreparedRule>& rules,
    const TVector<TRuleIndex>& ruleIndices,
    const TRuleParamOffset& regexListOffset
) {
    Entries.reserve(ruleIndices.size());

    std::visit([&] <typename T> (const T& offset) {
        if constexpr (std::is_same_v<T, TStringRegexListOffset> || std::is_same_v<T, THeaderRegexListOffset>) {
            for (const auto ruleIndex : ruleIndices) {
                Entries.push_back({
                    .RuleIndices = {ruleIndex},
                    .Scanner = offset.Access(rules[ruleIndex]).Scanner
                });
            }
        } else {
            Y_ENSURE(
                false,
                "TStringByRegexMatcher accepts offsets only of types TStringRegexListOffset and "
                "THeaderRegexListOffset"
            );
        }
    }, regexListOffset);

    GlueScanners(
        &Entries,
        [] (auto& entry) -> TEnumerableScanner& {
            return entry.Scanner;
        },
        [] (auto* src, auto* dst) {
            Copy(
                src->RuleIndices.begin(), src->RuleIndices.end(),
                std::back_inserter(dst->RuleIndices)
            );
        },
        5000
    );

    THashMap<TVector<TRuleIndex>, size_t, TRangeHash<>> ruleIndicesToState;
    TVector<TRuleIndex> accepted;

    for (auto& entry : Entries) {
        ruleIndicesToState.clear();

        for (const auto& state : entry.Scanner.EnumerateStates()) {
            if (!entry.Scanner.Final(state)) {
                continue;
            }

            const auto [acceptedBegin, acceptedEnd] = entry.Scanner.AcceptedRegexps(state);
            accepted.assign(acceptedBegin, acceptedEnd);

            Sort(accepted);

            const auto iState = static_cast<size_t>(state);
            const auto [remapIt, _] = ruleIndicesToState.insert({accepted, iState});
            entry.Remap[iState] = remapIt->second;
        }
    }
}

TVector<std::pair<size_t, TVector<TRuleIndex>>> TStringByRegexMatcher::EnumerateStates() const {
    TVector<std::pair<TState, TVector<TRuleIndex>>> states;

    for (const auto& entry : Entries) {
        for (const auto& state : entry.Scanner.EnumerateStates()) {
            if (!entry.Scanner.Final(state)) {
                continue;
            }

            const auto iState = static_cast<size_t>(state);
            if (*entry.Remap.FindPtr(iState) != iState) {
                continue;
            }

            auto [acceptedIt, acceptedEnd] = entry.Scanner.AcceptedRegexps(state);

            TVector<TRuleIndex> stateRuleIndices;
            stateRuleIndices.reserve(acceptedEnd - acceptedIt);

            for (; acceptedIt != acceptedEnd; ++acceptedIt) {
                stateRuleIndices.push_back(entry.RuleIndices[*acceptedIt]);
            }

            states.push_back({iState, std::move(stateRuleIndices)});
        }
    }

    return states;
}

void TStringByRegexMatcher::MatchStates(TStringBuf s, TVector<size_t>* out) const {
    for (const auto& entry : Entries) {
        const auto state = NPire::Runner(entry.Scanner).Run(s).State();
        if (entry.Scanner.Final(state)) {
            out->push_back(*entry.Remap.FindPtr(static_cast<size_t>(state)));
        }
    }
}

void TStringByRegexMatcher::MatchRules(TStringBuf s, TVector<TRuleIndex>* out) const {
    for (const auto& entry : Entries) {
        const auto state = NPire::Runner(entry.Scanner).Run(s).State();
        if (entry.Scanner.Final(state)) {
            auto [it, end] = entry.Scanner.AcceptedRegexps(state);
            for (; it != end; ++it) {
                out->push_back(entry.RuleIndices[*it]);
            }
        }
    }
}


template <typename EEnum>
TEnumByRegexMatcher<EEnum>::TEnumByRegexMatcher(
    const TVector<TPreparedRule>& rules,
    const TVector<TRuleIndex>& ruleIndices,
    TStringRegexListOffset regexListOffset
) {
    for (const auto ruleIndex : ruleIndices) {
        const auto& scanner = regexListOffset.Access(rules[ruleIndex]).Scanner;

        for (size_t i = 0; i < static_cast<size_t>(EEnum::Count); ++i) {
            const auto s = ToString(static_cast<EEnum>(i));
            if (scanner.Matches(s)) {
                EnumToRuleIndices[i].push_back(ruleIndex);
            }
        }
    }
}

template <typename EEnum>
TVector<std::pair<size_t, TVector<TRuleIndex>>> TEnumByRegexMatcher<EEnum>::EnumerateStates() const {
    TVector<std::pair<size_t, TVector<TRuleIndex>>> states;

    for (const auto& [state, ruleIndices] : Enumerate(EnumToRuleIndices)) {
        if (!ruleIndices.empty()) {
            states.push_back({state, ruleIndices});
        }
    }

    return states;
}

template <typename EEnum>
void TEnumByRegexMatcher<EEnum>::MatchStates(EEnum value, TVector<size_t>* out) const {
    if (!EnumToRuleIndices[static_cast<size_t>(value)].empty()) {
        out->push_back(static_cast<size_t>(value));
    }
}

template <typename EEnum>
void TEnumByRegexMatcher<EEnum>::MatchRules(EEnum value, TVector<TRuleIndex>* out) const {
    const auto& ruleIndices = EnumToRuleIndices[static_cast<size_t>(value)];
    Copy(ruleIndices.begin(), ruleIndices.end(), std::back_inserter(*out));
}

template class TEnumByRegexMatcher<EHostType>;
template class TEnumByRegexMatcher<EJwsState>;
template class TEnumByRegexMatcher<EYandexTrustState>;


TFinalChecker::TFinalChecker(
    TCbbGroupId groupId,
    const TPreparedRule& rule,
    THashMap<TCbbGroupId, TRefreshableAddrSet>* idToAddrList,
    TCbbIpListManager* ipListManager
) {
    Key = TCbbRuleKey(groupId, rule.RuleId);
    FillCheckerMasks(rule, CheckMask, TargetMask);
    IpInterval = rule.IpInterval;
    ExpBin = rule.ExpBin.Defined() ? *rule.ExpBin : EExpBin::Count;
    CookieAge = rule.CookieAge;
    CurrentTimestamp = rule.CurrentTimestamp;
    RandomThreshold = rule.RandomThreshold.GetOrElse(1);
    Factors = rule.PreparedFactors;
    Whitelisting = rule.Nonblock;

    if (ipListManager && rule.CbbGroup.Defined()) {
        const auto [idAddrList, inserted] = idToAddrList->insert({*rule.CbbGroup, {}});
        if (inserted) {
            idAddrList->second = ipListManager->Add({*rule.CbbGroup});
        }

        Addrs = idAddrList->second;
    }
}


bool TMatchContext::FinalCheck(TRuleIndex ruleIndex) {
    const auto& finalChecker = RuleSet->FinalCheckers[ruleIndex];

    if ((FlagMask & finalChecker.CheckMask) != finalChecker.TargetMask) {
        return false;
    }

    if (!finalChecker.IpInterval.IsEmpty() && !finalChecker.IpInterval.HasAddr(Req->UserAddr)) {
        return false;
    }

    if (finalChecker.ExpBin != EExpBin::Count && finalChecker.ExpBin != Req->ExperimentBin()) {
        return false;
    }

    if (!finalChecker.CookieAge.Compare(Req->CookieAge)) {
        return false;
    }

    if (
        !finalChecker.CurrentTimestamp.empty() &&
        !AllOf(
            finalChecker.CurrentTimestamp,
            [req = Req] (const auto& cmp) { return cmp.Compare(req->CurrentTimestamp); }
        )
    ) {
        return false;
    }

    if (finalChecker.RandomThreshold < 1 && RandomNumber<float>() >= finalChecker.RandomThreshold) {
        return false;
    }

    if (
        finalChecker.Addrs &&
        (AddrWhitelisted || !finalChecker.Addrs->Get()->ContainsActual(Req->UserAddr))
    ) {
        return false;
    }

    if (CacherFactors) {
        for (const auto& factor : finalChecker.Factors) {
            if (!factor.Comparator.Compare(CacherFactors->Factors[factor.FactorIndex])) {
                return false;
            }
        }
    } else if (!finalChecker.Factors.empty()) {
        return false;
    }

    for (const auto& [depth, count] : finalChecker.HeaderDepthsCounts) {
        if (count != HeaderCounts[depth]) {
            return false;
        }
    }

    for (const auto& groupEntry : finalChecker.GroupEntries) {
        const auto [cacheIt, inserted] = DepthIdToPatternListMatches.insert(
            {{groupEntry.Depth, groupEntry.Id}, false}
        );

        if (inserted) {
            const auto matcher = (*RuleSet->IdToPatternList.FindPtr(groupEntry.Id))->Get();
            const auto param = std::get<TStringBuf>(DepthToParam[groupEntry.Depth]);
            cacheIt->second =
                matcher->Defined() &&
                (*matcher)->Matches(param) == groupEntry.ShouldMatch;
        }

        if (!cacheIt->second) {
            return false;
        }
    }

    return true;
}

void TMatchContext::Push(TRuleIndex ruleIndex) {
    if (FinalCheck(ruleIndex)) {
        const auto& finalChecker = RuleSet->FinalCheckers[ruleIndex];
        Output.push_back({
            .Key = finalChecker.Key,
            .Whitelisting = finalChecker.Whitelisting,
            .ExpBin = finalChecker.ExpBin
        });
    }
}


// TODO(rzhikharevich): Clean up building code.

TRuleSetTreeNode::TRuleSetTreeNode(
    size_t depth,
    const TVector<TPreparedRule>& rules,
    const TVector<TRuleIndex>& ruleIndices,
    const TVector<TRuleParamOffset>& depthToRuleParamOffset
) {
    TVector<TRuleIndex> activeRuleIndices;
    TVector<TRuleIndex> inactiveRuleIndices;

    while (true) {
        activeRuleIndices.clear();
        inactiveRuleIndices.clear();

        std::visit([&] (const auto& offset) {
            for (const auto ruleIndex : ruleIndices) {
                if (offset.Access(rules[ruleIndex]).Scanner.Empty()) {
                    inactiveRuleIndices.push_back(ruleIndex);
                } else {
                    activeRuleIndices.push_back(ruleIndex);
                }
            }
        }, depthToRuleParamOffset[depth]);

        if (!activeRuleIndices.empty()) {
            break;
        }

        ++depth;

        if (depth >= depthToRuleParamOffset.size()) {
            Depth = depth;
            InactiveLeafRuleIndices = std::move(inactiveRuleIndices);
            return;
        }
    }

    Depth = depth;

    Matcher = std::visit([&] <typename T> (const T& offset) -> TMatcher {
        return typename T::TMatcher(rules, activeRuleIndices, offset);
    }, depthToRuleParamOffset[depth]);

    if (depth + 1 < depthToRuleParamOffset.size()) {
        const auto statesRuleIndices = std::visit([] (const auto& matcher) {
            return matcher.EnumerateStates();
        }, Matcher);
        StateToChild.reserve(statesRuleIndices.size());

        TVector<TRuleIndex> allRuleIndices;

        for (const auto& [state, stateRuleIndices] : statesRuleIndices) {
            allRuleIndices.clear();
            Copy(inactiveRuleIndices.begin(), inactiveRuleIndices.end(), std::back_inserter(allRuleIndices));
            Copy(stateRuleIndices.begin(), stateRuleIndices.end(), std::back_inserter(allRuleIndices));

            StateToChild.try_emplace(
                state,
                depth + 1,
                rules,
                allRuleIndices,
                depthToRuleParamOffset
            );
        }

        if (!inactiveRuleIndices.empty()) {
            Epsilon = MakeHolder<TRuleSetTreeNode>(
                depth + 1,
                rules,
                inactiveRuleIndices,
                depthToRuleParamOffset
            );
        }
    } else {
        InactiveLeafRuleIndices = std::move(inactiveRuleIndices);
    }
}

void TRuleSetTreeNode::Match(TMatchContext* ctx) const {
    for (const auto ruleIndex : InactiveLeafRuleIndices) {
        ctx->Push(ruleIndex);
    }

    if (Depth >= ctx->DepthToParam.size()) {
        return;
    }

    const auto param = ctx->DepthToParam[Depth];
    bool visitedChild = false;

    if (StateToChild.empty()) {
        ctx->TmpIndices.clear();

        std::visit([&] <typename T> (const T& matcher) {
            matcher.MatchRules(std::get<typename T::TValue>(param), &ctx->TmpIndices);
        }, Matcher);

        for (const auto ruleIndex : ctx->TmpIndices) {
            ctx->Push(ruleIndex);
        }
    } else {
        const auto begin = ctx->TmpStates.size();

        std::visit([&] <typename T> (const T& matcher) {
            matcher.MatchStates(std::get<typename T::TValue>(param), &ctx->TmpStates);
        }, Matcher);

        const auto end = ctx->TmpStates.size();

        for (size_t i = begin; i < end; ++i) {
            const auto child = StateToChild.FindPtr(ctx->TmpStates[i]);
            Y_ENSURE(child, "failed to find child for state");

            visitedChild = true;
            child->Match(ctx);
        }

        ctx->TmpStates.resize(begin);
    }

    if (Epsilon && !visitedChild) {
        Epsilon->Match(ctx);
    }
}


TRuleSet::TRuleSet(
    const TVector<std::pair<TCbbGroupId, TVector<TPreparedRule>>>& idsGroups,
    TRefreshableAddrSet nonBlockAddrs,
    TCbbIpListManager* maybeIpListManager,
    TCbbReListManager* maybeReListManager
)
    : NonBlockAddrs(std::move(nonBlockAddrs))
    , IpListManager(maybeIpListManager)
    , ReListManager(maybeReListManager)
{
    TVector<TPreparedRule> rules;
    THashSet<TString> headerNames;
    THashSet<TString> csHeaderNames;

    for (const auto& [groupId, group] : idsGroups) {
        for (const auto& rule : group) {
            if (!rule.Enabled || rule.Empty()) {
                continue;
            }

            TFinalChecker checker(groupId, rule, &IdToAddrList, IpListManager);

            FinalCheckers.push_back(std::move(checker));
            rules.push_back(rule);

            for (const auto& [key, _] : rule.Headers) {
                headerNames.insert(key);
            }

            for (const auto& hasHeader : rule.HasHeaders) {
                auto headerName = hasHeader.HeaderName;
                headerName.to_lower();
                headerNames.emplace(std::move(headerName));
            }

            for (const auto& numHeader : rule.NumHeaders) {
                auto headerName = numHeader.HeaderName;
                headerName.to_lower();
                headerNames.emplace(std::move(headerName));
            }

            for (const auto& hasCsHeader : rule.HasCsHeaders) {
                csHeaderNames.insert(hasCsHeader.HeaderName);
            }

            for (const auto& [key, _] : rule.CsHeaders) {
                csHeaderNames.insert(key);
            }

            for (const auto& numCsHeader : rule.NumCsHeaders) {
                csHeaderNames.insert(numCsHeader.HeaderName);
            }
        }
    }

    DepthToOffset = {STATIC_OFFSETS.begin(), STATIC_OFFSETS.end()};

    HeaderNames = {headerNames.begin(), headerNames.end()};
    CsHeaderNames = {csHeaderNames.begin(), csHeaderNames.end()};

    THashMap<TString, size_t> headerNameToDepth;
    headerNameToDepth.reserve(headerNames.size());

    for (const auto& headerName : HeaderNames) {
        DepthToOffset.push_back(THeaderRegexListOffset(
            &TPreparedRule::PreparedHeaders,
            headerName
        ));

        headerNameToDepth[headerName] = DepthToOffset.size() - 1;
    }

    THashMap<TString, size_t> csHeaderNameToDepth;
    csHeaderNameToDepth.reserve(CsHeaderNames.size());

    for (const auto& csHeaderName : CsHeaderNames) {
        DepthToOffset.push_back(THeaderRegexListOffset(
            &TPreparedRule::PreparedCsHeaders,
            csHeaderName
        ));

        csHeaderNameToDepth[csHeaderName] = DepthToOffset.size() - 1;
    }

    const size_t maxDepth = DepthToOffset.size();
    THashMap<TVector<size_t>, TVector<TRuleIndex>, TRangeHash<>> maskToRuleIndices;
    TVector<size_t> tmpMask;
    TVector<size_t> tmpHasHeaderMask;

    for (const auto& [ruleIndex, rule] : Enumerate(rules)) {
        tmpMask.clear();
        tmpHasHeaderMask.clear();

        auto& checker = FinalCheckers[ruleIndex];

        for (size_t depth = 0; depth < maxDepth; ++depth) {
            const auto& condition = std::visit([&rule = rule] (const auto& offset) {
                return offset.Access(rule);
            }, DepthToOffset[depth]);

            if (!condition.Empty()) {
                tmpMask.push_back(depth);

                const bool hasStrBuf = std::visit([] <typename T> (const T&) {
                    return std::is_same_v<typename T::TMatcher::TValue, TStringBuf>;
                }, DepthToOffset[depth]);

                if (hasStrBuf && ReListManager) {
                    for (const auto& groupCondition : condition.GroupConditions) {
                        if (
                            const auto [idPatternListIt, inserted] =
                                IdToPatternList.insert({groupCondition.GroupId, {}});
                            inserted
                        ) {
                            idPatternListIt->second = ReListManager->Add(groupCondition.GroupId);
                        }

                        checker.GroupEntries.push_back({
                            depth, groupCondition.GroupId,
                            groupCondition.ShouldMatch
                        });
                    }
                }
            }
        }

        for (const auto& hasHeader : rule.HasHeaders) {
            auto headerName = hasHeader.HeaderName;
            headerName.to_lower();
            const size_t depth = *headerNameToDepth.FindPtr(headerName);

            if (hasHeader.Present) {
                tmpHasHeaderMask.push_back(depth);
            } else {
                checker.HeaderDepthsCounts.push_back({depth - STATIC_OFFSETS.size(), 0});
            }
        }

        for (const auto& numHeader : rule.NumHeaders) {
            auto headerName = numHeader.HeaderName;
            headerName.to_lower();
            const size_t depth = *headerNameToDepth.FindPtr(headerName);

            if (numHeader.Count >= 1) {
                tmpHasHeaderMask.push_back(depth);
            }

            checker.HeaderDepthsCounts
                .push_back({depth - STATIC_OFFSETS.size(), numHeader.Count});
        }

        for (const auto& hasCsHeader : rule.HasCsHeaders) {
            const size_t depth = *csHeaderNameToDepth.FindPtr(hasCsHeader.HeaderName);

            if (hasCsHeader.Present) {
                tmpHasHeaderMask.push_back(depth);
            } else {
                checker.HeaderDepthsCounts.push_back({depth - STATIC_OFFSETS.size(), 0});
            }
        }

        for (const auto& numCsHeader : rule.NumCsHeaders) {
            const size_t depth = *csHeaderNameToDepth.FindPtr(numCsHeader.HeaderName);

            if (numCsHeader.Count >= 1) {
                tmpHasHeaderMask.push_back(depth);
            }

            checker.HeaderDepthsCounts
                .push_back({depth - STATIC_OFFSETS.size(), numCsHeader.Count});
        }

        SortUnique(tmpHasHeaderMask);
        Copy(tmpHasHeaderMask.begin(), tmpHasHeaderMask.end(), std::back_inserter(tmpMask));
        std::inplace_merge(tmpMask.begin(), tmpMask.end() - tmpHasHeaderMask.size(), tmpMask.end());
        tmpMask.erase(Unique(tmpMask.begin(), tmpMask.end()), tmpMask.end());

        maskToRuleIndices[tmpMask].push_back(ruleIndex);
    }

    Roots.reserve(maskToRuleIndices.size());

    for (const auto& [mask, ruleIndices] : maskToRuleIndices) {
        Roots.push_back({
            mask,
            TRuleSetTreeNode(0, rules, ruleIndices, DepthToOffset)
        });
    }
}

TVector<TMatchEntry> TRuleSet::Match(
    const TRequest& req,
    const TRawCacherFactors* cacherFactors
) const {
    TMatchContext ctx = {
        .RuleSet = this,
        .Req = &req,
        .CacherFactors = cacherFactors,
        .FlagMask = MakeFlagMask(req),
        .AddrWhitelisted = NonBlockAddrs && NonBlockAddrs->Get()->ContainsActual(req.UserAddr)
    };

    const auto uidString = ToString(req.Uid);
    const auto arrivalTimeString = ToString(req.ArrivalTime.MicroSeconds());
    const auto countryIdString = ToString(req.UserAddr.CountryId());

    std::initializer_list<TRequestParamValue> staticParams = {
        req.HostType,
        req.JwsState,
        req.YandexTrustState,
        uidString,
        req.Doc,
        req.Cgi,
        arrivalTimeString,
        req.Request,
        req.Hodor,
        req.HodorHash,
        countryIdString
    };

    ctx.DepthToParam.reserve(staticParams.size() + HeaderNames.size() + CsHeaderNames.size());

    Copy(staticParams.begin(), staticParams.end(), std::back_inserter(ctx.DepthToParam));

    ctx.HeaderCounts.reserve(HeaderNames.size() + CsHeaderNames.size());

    for (const auto& headerName : HeaderNames) {
        ctx.DepthToParam.push_back(req.Headers.Get(headerName));
        ctx.HeaderCounts.push_back(req.Headers.NumOfValues(headerName));
    }

    for (const auto& headerName : CsHeaderNames) {
        if (const auto it = req.CSHeaders.find(headerName); it != req.CSHeaders.end()) {
            ctx.DepthToParam.push_back(it->second);
        } else {
            ctx.DepthToParam.push_back(TStringBuf());
        }

        ctx.HeaderCounts.push_back(req.CSHeaders.count(headerName));
    }

    for (const auto& root : Roots) {
        if (AnyOf(root.Mask, [&] (size_t depth) {
            return
                depth >= staticParams.size() &&
                ctx.HeaderCounts[depth - staticParams.size()] == 0;
        })) {
            continue;
        }

        root.Root.Match(&ctx);
    }

    return std::move(ctx.Output);
}

void TRuleSet::RemoveGroups() const {
    for (const auto& [groupId, _] : IdToAddrList) {
        IpListManager->Remove({groupId});
    }

    for (const auto& [groupId, _] : IdToPatternList) {
        ReListManager->Remove({groupId});
    }
}


} // namespace NAntiRobot
