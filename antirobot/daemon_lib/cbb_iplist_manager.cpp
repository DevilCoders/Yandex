#include "cbb_iplist_manager.h"

#include "cbb.h"
#include "cbb_id.h"
#include "parse_cbb_response.h"

#include <library/cpp/containers/concurrent_hash/concurrent_hash.h>
#include <library/cpp/threading/rcu/rcu_accessor.h>

#include <util/digest/sequence.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/string/join.h>

#include <utility>


namespace NAntiRobot {


void TCbbIpListManagerCallback::operator()(const TVector<TString>& addrSetStrs) {
    TVector<TAddrSet> addrSets;
    addrSets.reserve(addrSetStrs.size());

    for (const auto& addrSetStr : addrSetStrs) {
        NParseCbb::ParseAddrList(addrSets.emplace_back(), addrSetStr);
    }

    Addrs->Set(TCbbAddrSet(MergeAddrSets(addrSets), Ids));
}


TRefreshableAddrSet TCbbIpListManager::Add(const TVector<TCbbGroupId>& ids) {
    auto [guard, callbackData] = TBase::Add(ids);
    auto& callback = callbackData->Callback;

    if (callback.Ids.empty()) {
        callback.Ids = ids;
    }

    return callback.Addrs;
}


} // namespace NAntiRobot
