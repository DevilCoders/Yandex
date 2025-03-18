#pragma once

#include "addr_list.h"
#include "cbb_list_manager.h"
#include "cbb.h"

#include <util/generic/vector.h>


namespace NAntiRobot {


struct TCbbIpListManagerRequester {
    TFuture<TString> operator()(
        ICbbIO* cbb,
        TCbbGroupId id
    ) const {
        return cbb->ReadList(id, "range_src,range_dst,expire");
    }
};


struct TCbbIpListManagerCallback : public TCbbListManagerCallback {
    TVector<TCbbGroupId> Ids;
    TRefreshableAddrSet Addrs = MakeRefreshableAddrSet();

    void operator()(const TVector<TString>& addrSets);
};


class TCbbIpListManager : public TCbbListManager<
    TCbbIpListManagerRequester,
    TCbbIpListManagerCallback
> {
private:
    using TBase = TCbbListManager<TCbbIpListManagerRequester, TCbbIpListManagerCallback>;

public:
    TRefreshableAddrSet Add(const TVector<TCbbGroupId>& ids);

    using TBase::Remove;
};


} // namespace NAntiRobot
