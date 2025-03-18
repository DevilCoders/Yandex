#pragma once

#include "addr.h"

namespace NAntiRobot {
    struct TIpInterval {
        inline TIpInterval(const TAddr& beg, const TAddr& end)
            : IpBeg(beg)
            , IpEnd(end)
        {
            if (!IpBeg.Valid() || !IpEnd.Valid() || IpEnd < IpBeg) {
                ythrow yexception() << "bad ip range(" << IpBeg << ", " << IpEnd << ")";
            }
        }

        static TIpInterval Parse(const TStringBuf& s);
        static TIpInterval MakeEmpty();

        TAddr IpBeg;
        TAddr IpEnd;

        inline bool operator<(const TIpInterval& other) const {
            return std::forward_as_tuple(IpBeg, IpEnd) < std::forward_as_tuple(other.IpBeg, other.IpEnd);
        }

        inline bool operator==(const TIpInterval& other) const {
            return std::forward_as_tuple(IpBeg, IpEnd) == std::forward_as_tuple(other.IpBeg, other.IpEnd);
        }

        inline bool operator< (const TAddr& addr) const {
            return IpEnd < addr;
        }

        TString Print() const;
        bool HasAddr(const TAddr& addr) const {
            return IpBeg <= addr && addr <= IpEnd;
        }
        bool IsEmpty() const {
            return IpBeg == TAddr::FromIp(0) && IpEnd == TAddr::FromIp(0);
        }
    };

} // namespace NAntiRobot
