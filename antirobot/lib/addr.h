#pragma once

#include <library/cpp/digest/sfh/sfh.h>

#include <util/network/address.h>

#include <cstring>

namespace NAntiRobot {
    class TAddr : public NAddr::TOpaqueAddr {
    public:
        TAddr() {
        }

        TAddr(const NAddr::TOpaqueAddr& rh)
            : NAddr::TOpaqueAddr(rh)
        {
        }

        TAddr(const TStringBuf& addrStr) {
            FromString(addrStr);
        }

        static TAddr FromIp(ui32 ip);
        static TAddr FromIp6(const TStringBuf& ip6);

        const sockaddr* Addr() const final {
            return NAddr::TOpaqueAddr::Addr();
        }

        socklen_t Len() const final {
            return NAddr::TOpaqueAddr::Len();
        }

        bool operator==(const TAddr& rhs) const {
            return Len() == rhs.Len() && !std::memcmp(Addr(), rhs.Addr(), Len());
        }

        bool operator!=(const TAddr& rhs) const {
            return !(*this == rhs);
        }

        bool operator<(const TAddr& rhs) const {
            if (Len() < rhs.Len()) {
                return true;
            }

            if (Len() > rhs.Len()) {
                return false;
            }

            return std::memcmp(Addr(), rhs.Addr(), Len()) < 0;
        }

        bool operator<=(const TAddr& rhs) const {
            if (Len() < rhs.Len()) {
                return true;
            }

            if (Len() > rhs.Len()) {
                return false;
            }

            return memcmp(Addr(), rhs.Addr(), Len()) <= 0;
        }

        bool Valid() const {
            return GetFamily() != 0;
        }

        bool IsMax() const;

        void FromString(const TStringBuf& addrStr);
        TString ToString() const;

        void Save(IOutputStream* out) const;
        void Load(IInputStream* in);

        size_t GetFamily() const;
        ui32 AsIp() const;
        TStringBuf AsIp6() const;

        bool IsIp4() const;
        bool IsIp6() const;

        TAddr GetSubnet(size_t subnetBitCount) const;

        void GetIntervalForMask(size_t numBits, TAddr& minAddr, TAddr& maxAddr) const;
        TAddr Next() const;
        TAddr Prev() const;

        ui32 GetProjId() const;

        inline ui32 Hash() const {
            return SuperFastHash(Addr(), Len());
        }

    private:
        TAddr NextPrev(i32 step, ui64 overflow) const;
    };

    struct TAddrHash {
        inline ui32 operator() (const TAddr& addr) const noexcept {
            return addr.Hash();
        }
    };

    size_t CalcIpDistance(const TAddr& addr1, const TAddr& addr2);
}
