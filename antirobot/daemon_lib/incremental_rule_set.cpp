#include "incremental_rule_set.h"

#include <util/generic/algorithm.h>


namespace NAntiRobot {


TIncrementalRuleSet::TIncrementalRuleSet(
    const TVector<std::pair<TCbbGroupId, TVector<TPreparedRule>>>& idsGroups,
    TRefreshableAddrSet nonBlockAddrs,
    TCbbIpListManager* maybeIpListManager,
    TCbbReListManager* maybeReListManager
)
    : NonBlockAddrs(nonBlockAddrs)
    , MaybeIpListManager(maybeIpListManager)
    , MaybeReListManager(maybeReListManager)
    , MergedRuleSet(idsGroups, std::move(nonBlockAddrs), maybeIpListManager, maybeReListManager)
{}

void TIncrementalRuleSet::Add(TCbbGroupId groupId, const TPreparedRule& rule) {
    const TWriteGuard guard(StateMutex);

    const TBinnedCbbRuleKey key{{groupId, rule.RuleId}, rule.ExpBin.GetOrElse(EExpBin::Count)};

    if (KeyToData.contains(key)) {
        return;
    }

    TRuleSet ruleSet({{groupId, {rule}}}, NonBlockAddrs, MaybeIpListManager, MaybeReListManager);
    UnmergedRuleSets.push_back({key, std::move(ruleSet)});
    KeyToData[key] = {.Rule = rule, .UnmergedIndex = UnmergedRuleSets.size() - 1};
    RemovedKeys.erase(key);
}

void TIncrementalRuleSet::Remove(TBinnedCbbRuleKey key) {
    const TWriteGuard guard(StateMutex);

    if (const auto keyData = KeyToData.find(key); keyData != KeyToData.end()) {
        const auto idx = keyData->second.UnmergedIndex;
        if (idx != INVALID_INDEX) {
            RemoveRuleSet(idx, keyData->second);
        }

        KeyToData.erase(keyData);
    }

    RemovedKeys.insert(key);
}

void TIncrementalRuleSet::Merge() {
    const auto mergeGuard = Guard(MergeMutex);

    THashMap<TCbbGroupId, TVector<TPreparedRule>> idToGroup;
    THashSet<TBinnedCbbRuleKey> removedKeys;
    THashSet<TBinnedCbbRuleKey> unmergedKeys;

    {
        TReadGuard guard(StateMutex);

        if (UnmergedRuleSets.empty() && RemovedKeys.empty()) {
            return;
        }

        for (const auto& [key, data] : KeyToData) {
            idToGroup[key.Key.Group].push_back(data.Rule);
        }

        removedKeys = RemovedKeys;

        for (const auto& [key, _] : KeyToData) {
            unmergedKeys.insert(key);
        }
    }

    const TVector<std::pair<TCbbGroupId, TVector<TPreparedRule>>> idsGroups(
        std::make_move_iterator(idToGroup.begin()),
        std::make_move_iterator(idToGroup.end())
    );

    TRuleSet mergedRuleSet(idsGroups, NonBlockAddrs, MaybeIpListManager, MaybeReListManager);

    {
        TWriteGuard guard(StateMutex);

        MergedRuleSet.RemoveGroups();
        MergedRuleSet = std::move(mergedRuleSet);

        for (const auto key : removedKeys) {
            RemovedKeys.erase(key);
        }

        for (auto unmergedIt = UnmergedRuleSets.begin(); unmergedIt != UnmergedRuleSets.end();) {
            if (unmergedKeys.contains(unmergedIt->first)) {
                RemoveRuleSet(unmergedIt - UnmergedRuleSets.begin(), KeyToData[unmergedIt->first]);
            } else {
                ++unmergedIt;
            }
        }
    }
}

TVector<TMatchEntry> TIncrementalRuleSet::Match(
    const TRequest& req,
    const TRawCacherFactors* cacherFactors
) const {
    const TReadGuard guard(StateMutex);

    auto result = MergedRuleSet.Match(req, cacherFactors);

    EraseIf(result, [this] (const auto& entry) {
        return RemovedKeys.contains(entry.ToBinnedKey());
    });

    for (const auto& [_, ruleSet] : UnmergedRuleSets) {
        const auto subresult = ruleSet.Match(req, cacherFactors);
        if (!subresult.empty()) {
            result.push_back(subresult[0]);
        }
    }

    return result;
}

void TIncrementalRuleSet::RemoveRuleSet(size_t idx, TRuleData& data) {
    if (idx != UnmergedRuleSets.size() - 1) {
        auto& mid = UnmergedRuleSets[idx];
        std::swap(mid, UnmergedRuleSets.back());
        KeyToData[mid.first].UnmergedIndex = idx;
    }

    UnmergedRuleSets.back().second.RemoveGroups();
    UnmergedRuleSets.pop_back();
    data.UnmergedIndex = INVALID_INDEX;
}


} // namespace NAntiRobot
