#pragma once

#include "addr.h"
#include "ip_interval.h"

#include <util/generic/map.h>
#include <util/generic/maybe.h>

namespace NAntiRobot {

    template <class TData>
    class TIpRangeMap {
    public:
        void Insert(const std::pair<TIpInterval, TData>& item) {
            Y_ENSURE(!IpMap.contains(item.first) || IpMap[item.first] == item.second,
                     "Map already contains " << item.first.Print() << " with another value. ");

            IpMap.insert(item);
        }

        size_t Size() const {
            return IpMap.size();
        }

        void Finish() {
        }

        TMaybe<TData> Find(const TAddr& address) const {
            if (IpMap.empty()) {
                return Nothing();
            }

            auto it = IpMap.upper_bound(TIpInterval{address, address});
            if (it == IpMap.begin() && it->first.IpBeg != address) {
                return Nothing();
            }

            if (it == IpMap.end() || it->first.IpBeg != address) {
                it--;
            }

            if (address < it->first.IpBeg || it->first.IpEnd < address) {
                return Nothing();
            }

            return it->second;
        }

        void EnsureNoIntersections() const {
            if (IpMap.size() < 2) {
                return;
            }

            for (auto previous = IpMap.cbegin(), current = next(previous);
                 current != IpMap.cend();
                 previous++, current++) {
                Y_ENSURE(previous->first.IpEnd < current->first.IpBeg,
                         "Found intersection "
                             << previous->first.Print() << " and "
                             << current->first.Print());
            }
        }

    private:
        TMap<TIpInterval, TData> IpMap;
    };
}
