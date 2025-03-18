#pragma once

#include "cbb_id.h"

#include <antirobot/lib/addr.h>
#include <antirobot/lib/ip_list.h>

#include <library/cpp/threading/rcu/rcu_accessor.h>

#include <util/generic/map.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>

#include <utility>

namespace NAntiRobot {

    struct TAddrAttr {
        TAddr EndAddr;
        TInstant Expire;
    };

    class TAddrSet : private TMap<TAddr, TAddrAttr> {
        using TBase = TMap<TAddr, TAddrAttr>;
        using TIter = TBase::const_iterator;
    private:
        bool IsIn(const TAddr& addr, TIter iter) const {
            return iter != end() && iter->first <= addr && addr <= iter->second.EndAddr;
        }

        bool IntersectsWith(const TIpInterval& interval) const;

    public:
        bool ContainsActual(const TAddr& addr, TInstant time = TInstant::Now()) const {
            const auto it = Find(addr);
            return it != end() && it->second.Expire > time;
        }

        // If ipInterval intersects with any previously inserted interval then
        // it is not inserted and false is returned. Otherwise returns true.
        bool Add(const TIpInterval& ipInterval, TInstant expire);
        TIter Find(const TAddr& addr) const;

        using TBase::clear;
        using TBase::size;
        using TBase::begin;
        using TBase::end;
    };

    struct TCbbAddrSet : public TAddrSet {
        TVector<TCbbGroupId> CbbGroups = {};

        TCbbAddrSet() = default;

        TCbbAddrSet(
            TAddrSet addrSet,
            TVector<TCbbGroupId> cbbGroups
        )
            : TAddrSet(std::move(addrSet))
            , CbbGroups(std::move(cbbGroups))
        {}
    };

    using TRefreshableAddrSet = TAtomicSharedPtr<NThreading::TRcuAccessor<TCbbAddrSet>>;

    inline TRefreshableAddrSet MakeRefreshableAddrSet() {
        return new NThreading::TRcuAccessor<TCbbAddrSet>;
    }

    TAddrSet MergeAddrSets(
        const TVector<TAddrSet>& addrSets
    );
}
