#pragma once

#include "cbb_list_manager.h"
#include "cbb.h"
#include "incremental_rule_set.h"
#include "stat.h"

#include <library/cpp/threading/rcu/rcu_accessor.h>

#include <util/generic/hash_set.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/variant.h>
#include <util/generic/vector.h>


namespace NAntiRobot {


struct TEnv;


struct TCbbTextListManagerRequester {
    TFuture<TString> operator()(
        ICbbIO* cbb,
        TCbbGroupId id
    ) const {
        return cbb->ReadList(id, "range_txt,rule_id");
    }
};


struct TCbbTextListManagerCallback : public TCbbListManagerCallback {
public:
    TVector<TCbbGroupId> Ids;
    TString Title;
    TVector<TIncrementalRuleSet*> Matchers;

    TVector<THashSet<TBinnedCbbRuleKey>> CurrentRuleKeys;

    const TString& GetTitle() const {
        return Title;
    }

    void Merge() {
        for (auto* matcher : Matchers) {
            matcher->Merge();
        }
    }

    void operator()(const TVector<TString>& listStrs);

private:
    void Do(
        const TVector<TVector<TString>>& lists,
        const TString& title,
        TRefreshableAddrSet nonBlockingAddrs,
        const TVector<NThreading::TRcuAccessor<TRuleSet>*>& matchers
    );
};


class TCbbTextListManager : public TCbbListManager<
    TCbbTextListManagerRequester,
    TCbbTextListManagerCallback
> {
private:
    using TBase = TCbbListManager<TCbbTextListManagerRequester, TCbbTextListManagerCallback>;

public:
    void Add(
        const TVector<TCbbGroupId>& ids,
        const TString& title,
        TIncrementalRuleSet* matcher
    );
};


} // namespace NAntiRobot
