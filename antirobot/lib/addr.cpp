#include "addr.h"

#include "ar_utils.h"

#include <util/ysaveload.h>
#include <util/generic/algorithm.h>
#include <util/generic/bitops.h>
#include <util/generic/ylimits.h>
#include <util/stream/format.h>
#include <util/stream/str.h>
#include <util/string/printf.h>
#include <util/system/byteorder.h>
#include <util/system/defaults.h>
#include <util/system/unaligned_mem.h>

using namespace NAddr;

template <>
void Out<NAntiRobot::TAddr>(IOutputStream& out, const NAntiRobot::TAddr& addr) {
    size_t family = addr.GetFamily();
    if (family == AF_INET || family == AF_INET6)
        PrintHost(out, addr);
    else
        out << '-';
}

namespace NAntiRobot {
    namespace {
        const ui8 IPV4_MAPPED_PREFIX[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF };

        inline bool ParseIPv4(TOpaqueAddr* addr, const char* str) noexcept {
            sockaddr_in* sa = (sockaddr_in*)addr->Addr();

            if (inet_pton(AF_INET, str, &sa->sin_addr) > 0) {
#if defined(_freebsd_) || defined(_darwin_)
                sa->sin_len = sizeof(sockaddr_in);
#endif
                sa->sin_family = AF_INET;
                *addr->LenPtr() = sizeof(*sa);

                return true;
            }

            return false;
        }

        inline void AssignIPv4(TOpaqueAddr* addr, ui32 ip4) noexcept {
            sockaddr_in* sa = (sockaddr_in*)addr->Addr();

#if defined(_freebsd_) || defined(_darwin_)
            sa->sin_len = sizeof(sockaddr_in);
#endif
            sa->sin_addr.s_addr = ip4;
            sa->sin_family = AF_INET;
            *addr->LenPtr() = sizeof(*sa);
        }

        inline void AssignIPv6(TOpaqueAddr* addr, const TStringBuf& ip6) noexcept {
            sockaddr_in6* sa = (sockaddr_in6*)addr->Addr();

#if defined(_freebsd_) || defined(_darwin_)
            sa->sin6_len = sizeof(sockaddr_in6);
#endif
            sa->sin6_family = AF_INET6;
            memcpy(&sa->sin6_addr, ip6.data(), ip6.size());
            *addr->LenPtr() = sizeof(*sa);
        }

        inline size_t CalcIp4Distance(ui32 ip1, ui32 ip2) {
            const ui32 diff = ip1 ^ ip2;
            return diff ? GetValueBitCount(diff) : 0;
        }

        size_t GetValueBitCountIPv6(const TStringBuf& data)
        {
            ui64 val = HostToInet(*(reinterpret_cast<const ui64*>(data.data())));
            if (val)
                return GetValueBitCount(val) + 64;

            val = HostToInet(*(reinterpret_cast<const ui64*>(data.data() + 8)));
            return GetValueBitCount(val);
        }

        size_t CalcIp6Distance(const TStringBuf& addr1, const TStringBuf& addr2)
        {
            ui64 a = HostToInet(*(reinterpret_cast<const ui64*>(addr1.data())));
            ui64 b = HostToInet(*(reinterpret_cast<const ui64*>(addr2.data())));

            ui64 diff = a ^ b;
            if (diff)
                return GetValueBitCount(diff) + 64;

            a = HostToInet(*(reinterpret_cast<const ui64*>(addr1.data() + 8)));
            b = HostToInet(*(reinterpret_cast<const ui64*>(addr2.data() + 8)));

            diff = *reinterpret_cast<const ui64*>(addr1.data() + 8) ^ *reinterpret_cast<const ui64*>(addr2.data() + 8);
            if (diff)
                return GetValueBitCount(diff);

            return 0;
        }

        void ConvertToIp6(const TAddr& addr, TTempBuf& buf)
        {
            //
            // IPv4 mapped to IPv6 looks like
            // 0000:0000:0000:0000:0000:FFFF:XX.XX.XX.XX
            //

            buf.Reset();
            buf.Proceed(10);
            memset(buf.Data(), 0, 10);
            buf.Append("\xFF\xFF", 2);
            const ui32 ip = addr.AsIp();
            buf.Append(&ip, 4);
        }
    }

    static inline bool ParseIPv6(TOpaqueAddr* addr, const char* str) noexcept {
        sockaddr_in6* sa = (sockaddr_in6*)addr->Addr();

        if (inet_pton(AF_INET6, str, &sa->sin6_addr) > 0) {
#if defined(_freebsd_) || defined(_darwin_)
            sa->sin6_len = sizeof(sockaddr_in6);
#endif
            //
            // Check if IPv6 addr is IPv4 mapped to IPv6.
            // If so, return it as IPv4
            //
            static_assert(sizeof(IPV4_MAPPED_PREFIX) + sizeof(ui32) == sizeof(sa->sin6_addr.s6_addr), "expect sizeof(IPV4_MAPPED_PREFIX) + sizeof(ui32) == sizeof(sa->sin6_addr.s6_addr)");
            const size_t IPV4_OFFSET = sizeof(IPV4_MAPPED_PREFIX);

            const ui8* addrBytes = sa->sin6_addr.s6_addr;
            if (memcmp(addrBytes, IPV4_MAPPED_PREFIX, sizeof(IPV4_MAPPED_PREFIX)) == 0) {
                AssignIPv4(addr, *reinterpret_cast<const ui32*>(addrBytes + IPV4_OFFSET));
                return true;
            }

            sa->sin6_family = AF_INET6;
            *addr->LenPtr() = sizeof(*sa);

            return true;
        }

        return false;
    }

    static inline bool Parse(TOpaqueAddr* addr, TStringBuf str) noexcept {
        char buf[128];

        buf[str.copy(buf, sizeof(buf) - 1)] = 0;

        if (ParseIPv4(addr, buf)) {
            return true;
        }

        return ParseIPv6(addr, buf);
    }

    bool TAddr::IsMax() const {
        switch (GetFamily()) {
        case AF_INET: {
            const auto addr = reinterpret_cast<const sockaddr_in*>(Addr());
            return addr->sin_addr.s_addr == Max<ui32>();
        }

        case AF_INET6: {
            const auto addr = reinterpret_cast<const sockaddr_in6*>(Addr());

            for (size_t i = 0; i < 16; ++i) {
                if (addr->sin6_addr.s6_addr[i] != 0xFF) {
                    return false;
                }
            }

            return true;
        }

        default:
            Y_ENSURE(false, "Unsupported address family: " << GetFamily());
        }
    }

    void TAddr::FromString(const TStringBuf& addrStr) {
        if (!Parse(this, addrStr)) {
            *this = TAddr();
        }
    }

    TAddr TAddr::FromIp(ui32 ip) {
        NAddr::TOpaqueAddr addr;
        AssignIPv4(&addr, HostToInet(ip));
        return TAddr(addr);
    }

    TAddr TAddr::FromIp6(const TStringBuf& ip6) {
        NAddr::TOpaqueAddr addr;

        AssignIPv6(&addr, ip6);
        return TAddr(addr);
    }

    TString TAddr::ToString() const {
        TStringStream str;
        PrintHost(str, *this);
        return str.Str();
    }

    void TAddr::Save(IOutputStream* out) const {
        ::SaveSize(out, Len());
        ::SaveRange(out, (const char*)Addr(), (const char*)Addr() + Len());
    }

    void TAddr::Load(IInputStream* in) {
        *this = TAddr();
        *LenPtr() = ::LoadSize(in);
        ::LoadRange(in, (char*)Addr(), (char*)Addr() + Len());
    }

    size_t TAddr::GetFamily() const {
        return Addr()->sa_family;
    }

    ui32 TAddr::AsIp() const {
        if (GetFamily() != AF_INET)
            return 0;
        return InetToHost(((const sockaddr_in*)Addr())->sin_addr.s_addr);
    }

    TStringBuf TAddr::AsIp6() const {
        if (GetFamily() != AF_INET6)
            return TStringBuf();
        static_assert(sizeof(in6_addr) == 16, "expect sizeof(in6_addr) == 16");
        return TStringBuf((const char*)&((const sockaddr_in6*)Addr())->sin6_addr, sizeof(in6_addr));
    }

    void TAddr::GetIntervalForMask(size_t numBits, TAddr& minAddr, TAddr& maxAddr) const {
        switch (GetFamily()) {
        case AF_INET: {
                ui32 mask = numBits ? HostToInet(~((1u << (32 - Min<size_t>(numBits, 32))) - 1u)) : 0;
                minAddr = *this;
                ((sockaddr_in*)minAddr.Addr())->sin_addr.s_addr &= mask;
                maxAddr = *this;
                ((sockaddr_in*)maxAddr.Addr())->sin_addr.s_addr |= ~mask;
                break;
            }

        case AF_INET6: {
                minAddr = *this;
                maxAddr = *this;
                ui64* minAddrBuf = (ui64*)&((const sockaddr_in6*)minAddr.Addr())->sin6_addr;
                ui64* maxAddrBuf = (ui64*)&((const sockaddr_in6*)maxAddr.Addr())->sin6_addr;
                if (numBits <= 64) {
                    ui64 mask = numBits ? HostToInet(~((1ull << (64 - numBits)) - 1ull)) : 0;
                    *minAddrBuf &= mask;
                    *(minAddrBuf + 1) = 0;
                    *maxAddrBuf |= ~mask;
                    *(maxAddrBuf + 1) = ~0ull;
                } else {
                    ui64 mask = HostToInet(~((1ull << (128 - Min<size_t>(numBits, 128))) - 1ull));
                    *(minAddrBuf + 1) &= mask;
                    *(maxAddrBuf + 1) |= ~mask;
                }
                break;
            }

        default:
            ythrow yexception() << "Unsupported address family: " << GetFamily();
        }
    }

    TAddr TAddr::Next() const {
        return NextPrev(1, 0);
    }

    TAddr TAddr::Prev() const {
        return NextPrev(-1, Max<ui64>());
    }

    bool TAddr::IsIp4() const {
        return GetFamily() == AF_INET;
    }

    bool TAddr::IsIp6() const {
        return GetFamily() == AF_INET6;
    }

    TAddr TAddr::GetSubnet(size_t subnetBitCount) const {
        TAddr min, max;
        GetIntervalForMask(subnetBitCount, min, max);
        return min;
    }

    ui32 TAddr::GetProjId() const {
        const auto addr = (reinterpret_cast<const sockaddr_in6*>(Addr()))->sin6_addr;
        // Darwin don't have s6_addr32 member in union.
        return ReadUnaligned<ui32>(addr.s6_addr + 8);
    }

    TAddr TAddr::NextPrev(i32 step, ui64 overflow) const {
        TAddr res = *this;

        switch (GetFamily()) {
        case AF_INET: {
                ((sockaddr_in*)res.Addr())->sin_addr.s_addr = InetToHost(AsIp() + step);
                break;
            }

        case AF_INET6: {
                ui64* resBuf = (ui64*)&((const sockaddr_in6*)res.Addr())->sin6_addr;
                ui64& low = *(resBuf + 1);
                low = HostToInet(InetToHost(low) + step);
                if (low == overflow)
                    *resBuf = HostToInet(InetToHost(*resBuf) + step);
                break;
            }

        default:
            ythrow yexception() << "Unsupported address family: " << GetFamily();
        }

        return res;
    }

    size_t CalcIpDistance(const TAddr& addr1, const TAddr& addr2) {
        switch (addr1.GetFamily()) {
            case 0:
                switch(addr2.GetFamily()) {
                    case 0: return 0;
                    case AF_INET: return CalcIp4Distance(0, addr2.AsIp());
                    case AF_INET6: return GetValueBitCountIPv6(addr2.AsIp6());
                }
                break;

            case AF_INET:
                switch(addr2.GetFamily()) {
                    case 0: return CalcIp4Distance(addr1.AsIp(), 0);
                    case AF_INET: return CalcIp4Distance(addr1.AsIp(), addr2.AsIp());
                    case AF_INET6: {
                        TTempBuf buf(16);
                        ConvertToIp6(addr1, buf);
                        return CalcIp6Distance(TStringBuf(buf.Data(), buf.Filled()), addr2.AsIp6());
                    }
                }
                break;

            case AF_INET6:
                switch(addr2.GetFamily()) {
                    case 0: return GetValueBitCountIPv6(addr1.AsIp6());
                    case AF_INET: {
                        TTempBuf buf(16);
                        ConvertToIp6(addr2, buf);
                        return CalcIp6Distance(addr1.AsIp6(), TStringBuf(buf.Data(), buf.Filled()));
                    }
                    case AF_INET6: return CalcIp6Distance(addr1.AsIp6(), addr2.AsIp6());
                }
        }
        return 0;
    }
}

