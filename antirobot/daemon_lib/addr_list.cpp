#include "addr_list.h"
#include "cbb_id.h"

#include <library/cpp/containers/heap_dict/heap_dict.h>

#include <util/generic/algorithm.h>

#include <tuple>


namespace NAntiRobot {
    TAddrSet::TIter TAddrSet::Find(const TAddr& addr) const {
        auto it = upper_bound(addr);
        if (it != begin()) {
            --it;
            return IsIn(addr, it) ? it : end();
        }
        return end();
    }

    bool TAddrSet::Add(const TIpInterval& interval, TInstant expire) {
        if (IntersectsWith(interval)) {
            return false;
        }

        emplace(interval.IpBeg, TAddrAttr{interval.IpEnd, expire});
        return true;
    }

    bool TAddrSet::IntersectsWith(const TIpInterval& interv) const {
        auto boundBeg = upper_bound(interv.IpBeg);
        auto boundEnd = interv.IpBeg == interv.IpEnd ? boundBeg : upper_bound(interv.IpEnd);

        if (boundBeg == boundEnd) {
            if (boundBeg == begin()) {
                return false;
            }

            if (!IsIn(interv.IpBeg, --boundBeg)) {
                return false;
            }
        }

        return true;
    }

    using TAddrSetInterval = std::pair<const TAddr, TAddrAttr>;

    struct TAddrSetIntervalEvent {
        bool Added;
        const TAddrSetInterval* Interval;

        bool operator<(const TAddrSetIntervalEvent& that) const {
            if (Added) {
                if (that.Added) {
                    return Interval->first < that.Interval->first;
                } else {
                    return Interval->first <= that.Interval->second.EndAddr;
                }
            } else {
                if (that.Added) {
                    return Interval->second.EndAddr < that.Interval->first;
                } else {
                    return
                        Interval->second.EndAddr <
                        that.Interval->second.EndAddr;
                }
            }
        }
    };

    TAddrSet MergeAddrSets(
        const TVector<TAddrSet>& addrSets
    ) {
        size_t numIntervals = 0;

        for (const auto& addrSet : addrSets) {
            numIntervals += addrSet.size();
        }

        TVector<TAddrSetIntervalEvent> events;
        events.reserve(2 * numIntervals);

        for (const auto& addrSet : addrSets) {
            for (const auto& interval : addrSet) {
                events.push_back(TAddrSetIntervalEvent{true, &interval});
                events.push_back(TAddrSetIntervalEvent{false, &interval});
            }
        }

        Sort(events);

        struct TLessByExpire {
            bool operator()(
                const TAddrSetInterval* left,
                const TAddrSetInterval* right
            ) const {
                return
                    std::make_tuple(left->second.Expire, right->first) <
                    std::make_tuple(right->second.Expire, left->first);
            }
        };

        THeapDict<
            const TAddrSetInterval*, const TAddrSetInterval*,
            TLessByExpire
        > lowToIntervalByExpire;
        TAddr low;
        bool overflow = false;

        TAddrSet ret;

        for (const auto& event : events) {
            const auto& interval = *event.Interval;

            if (event.Added) {
                if (lowToIntervalByExpire.empty()) {
                    low = interval.first;
                    overflow = false;
                } else {
                    const auto& topInterval =
                        *lowToIntervalByExpire.top().second;

                    if (
                        interval.second.Expire > topInterval.second.Expire &&
                        low < interval.first
                    ) {
                        ret.Add(
                            TIpInterval(low, interval.first.Prev()),
                            topInterval.second.Expire
                        );

                        low = interval.first;
                        overflow = false;
                    }
                }

                lowToIntervalByExpire.push(&interval, &interval);
            } else {
                if (
                    &interval == lowToIntervalByExpire.top().second &&
                    !overflow && low <= interval.second.EndAddr
                ) {
                    ret.Add(
                        TIpInterval(low, interval.second.EndAddr),
                        interval.second.Expire
                    );

                    if (interval.second.EndAddr.IsMax()) {
                        overflow = true;
                    } else {
                        low = interval.second.EndAddr.Next();
                    }
                }

                lowToIntervalByExpire.erase(&interval);
            }
        }

        return ret;
    }
}
