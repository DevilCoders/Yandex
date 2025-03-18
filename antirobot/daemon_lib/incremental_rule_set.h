#pragma once

#include "addr_list.h"
#include "cbb_id.h"
#include "prepared_rule.h"
#include "rule_set.h"

#include <util/generic/hash_set.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/system/mutex.h>

#include <utility>


namespace NAntiRobot {


class TCbbIpListManager;
class TCbbReListManager;


class TIncrementalRuleSet {
public:
    TIncrementalRuleSet() = default;

    explicit TIncrementalRuleSet(
        const TVector<std::pair<TCbbGroupId, TVector<TPreparedRule>>>& idsGroups,
        TRefreshableAddrSet nonBlockAddrs = nullptr,
        TCbbIpListManager* maybeIpListManager = nullptr,
        TCbbReListManager* maybeReListManager = nullptr
    );

    void Reset(
        TRefreshableAddrSet nonBlockAddrs,
        TCbbIpListManager* maybeIpListManager,
        TCbbReListManager* maybeReListManager
    ) {
        NonBlockAddrs = std::move(nonBlockAddrs);
        MaybeIpListManager = maybeIpListManager;
        MaybeReListManager = maybeReListManager;
    }

    void Add(TCbbGroupId groupId, const TPreparedRule& rule);
    void Remove(TBinnedCbbRuleKey key);
    void Merge();

    TVector<TMatchEntry> Match(
        const TRequest& req,
        const TRawCacherFactors* cacherFactors = nullptr
    ) const;

private:
    struct TRuleData {
        TPreparedRule Rule;
        size_t UnmergedIndex = INVALID_INDEX;
    };

private:
    void RemoveRuleSet(size_t idx, TRuleData& data);

private:
    static constexpr size_t INVALID_INDEX = Max<size_t>();

    TRefreshableAddrSet NonBlockAddrs;
    TCbbIpListManager* MaybeIpListManager = nullptr;
    TCbbReListManager* MaybeReListManager = nullptr;

    TRWMutex StateMutex;
    TMutex MergeMutex;
    THashMap<TBinnedCbbRuleKey, TRuleData> KeyToData;
    THashSet<TBinnedCbbRuleKey> RemovedKeys;
    TVector<std::pair<TBinnedCbbRuleKey, TRuleSet>> UnmergedRuleSets;
    TRuleSet MergedRuleSet;
};


} // namespace NAntiRobot
