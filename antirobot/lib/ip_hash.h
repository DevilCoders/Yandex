#pragma once

#include "addr.h"
#include "ip_interval.h"

#include <util/generic/hash.h>
#include <util/generic/maybe.h>

namespace NAntiRobot {

template<class TData, size_t v4mask, size_t v6mask>
class TIpHashMap {
private:
    template<size_t>
    struct TTraits {
    };

    template<>
    struct TTraits<64> {
        using TKeyType = ui64;
    };

    template<>
    struct TTraits<32> {
        using TKeyType = ui32;
    };

    using TKeyType = typename TTraits<v6mask>::TKeyType;

public:
    constexpr size_t Size() const {
        return Ip4Data.size() + Ip6Data.size();
    }

    inline TMaybe<TData> Find(const TAddr& addr) const {
        if (addr.IsIp4()) {
            if (const auto* res = Ip4Data.FindPtr(addr.GetSubnet(v4mask).AsIp()); res) {
                return *res;
            }
        } else {
            if (const auto* res = Ip6Data.FindPtr(GetV6Addr(addr)); res) {
                return *res;
            }
        }
        return Nothing();
    }

    inline void Insert(const std::pair<TIpInterval, TData>& item) {
        const auto& addr = item.first.IpBeg;
        if (addr.IsIp4()) {
            Ip4Data[addr.GetSubnet(v4mask).AsIp()] = item.second;
        } else {
            Ip6Data[GetV6Addr(addr)] = item.second;
        }
    }

    inline void Finish() {
    }

private:
    inline static TKeyType GetV6Addr(const TAddr& addr) {
        const auto ip6 = (reinterpret_cast<const sockaddr_in6*>(addr.Addr()))->sin6_addr;
        // Darwin don't have s6_addr32 member in union.
        return ReadUnaligned<TKeyType>(ip6.s6_addr);
    }

    THashMap<ui32, TData> Ip4Data;
    THashMap<TKeyType, TData> Ip6Data;
};

} // namespace NAntiRobot
