#pragma once

#include "addr.h"
#include "ip_interval.h"

#include <util/generic/vector.h>
#include <util/generic/maybe.h>

namespace NAntiRobot {

template<class TData, size_t v4mask, size_t v6mask>
class TIpVector {
public:
    constexpr size_t Size() const {
        return Ip4Data.size() + Ip6Data.size();
    }

    inline TMaybe<TData> Find(const TAddr& addr) const {
        if (addr.IsIp4()) {
            const ui32 ip = addr.GetSubnet(v4mask).AsIp();
            const auto& it = LowerBound(begin(Ip4Data), end(Ip4Data), ip,
                [](const std::pair<ui32, TData>& elem, const ui32 ip) {
                    return elem.first < ip;
                });

            if (it != end(Ip4Data) && it->first == ip) {
                return it->second;
            }
        } else {
            const ui64 ip = GetV6Addr(addr);
            const auto& it = LowerBound(begin(Ip6Data), end(Ip6Data), ip,
                [](const std::pair<ui64, TData>& elem, const ui64 ip) {
                    return elem.first < ip;
                });

            if (it != end(Ip6Data) && it->first == ip) {
                return it->second;
            }
        }

        return Nothing();
    }

    inline void Insert(const std::pair<TIpInterval, TData>& item) {
        const auto& addr = item.first.IpBeg;
        if (addr.IsIp4()) {
            const ui32 ip = addr.GetSubnet(v4mask).AsIp();
            Ip4Data.push_back({ip, item.second});
        } else {
            Ip6Data.push_back({GetV6Addr(addr), item.second});
        }
    }

    inline void Finish() {
        Ip4Data.shrink_to_fit();
        Sort(Ip4Data,
            [](const std::pair<ui32, TData>& lhs, const std::pair<ui32, TData>& rhs) {
                return lhs.first < rhs.first;
            });

        Ip6Data.shrink_to_fit();
        Sort(Ip6Data,
            [](const std::pair<ui64, TData>& lhs, const std::pair<ui64, TData>& rhs) {
                return lhs.first < rhs.first;
            });
    }

private:

    inline static ui64 GetV6Addr(const TAddr& addr) {
        const auto ip6 = (reinterpret_cast<const sockaddr_in6*>(addr.Addr()))->sin6_addr;
        // Darwin don't have s6_addr32 member in union.
        return ReadUnaligned<ui64>(ip6.s6_addr);
    }

    TVector<std::pair<ui32, TData>> Ip4Data;
    TVector<std::pair<ui64, TData>> Ip6Data;
};

} // namespace NAntiRobot
