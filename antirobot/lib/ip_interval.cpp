#include "ip_interval.h"

#include <util/string/strip.h>
#include <util/string/cast.h>

namespace NAntiRobot {
    TIpInterval TIpInterval::Parse(const TStringBuf& l) {
        TStringBuf f;
        TStringBuf s;

        if (l.TrySplit('/', f, s)) {
            const TAddr addr(StripString(f));
            Y_ENSURE(addr.Valid(), "Cannot parse IP address " << f << " of network " << s);

            size_t numBits = FromString<size_t>(StripString(s).Before(' ').Before('\t'));

            Y_ENSURE(0 <= numBits, "Expected non-negative number as a network mask size, but got "
                    << numBits << "; network: " << s);

            if (addr.IsIp4()) {
                Y_ENSURE(numBits <= 32, "IpV4 network mask size should not be greater than 32; mask size: "
                        << numBits << "; network: " << s);
            } else {
                Y_ENSURE(numBits <= 128, "IpV6 network mask size should not be greater than 128; mask size: "
                        << numBits << "; network: " << s);
            }

            TAddr minAddr;
            TAddr maxAddr;
            addr.GetIntervalForMask(numBits, minAddr, maxAddr);
            return TIpInterval(minAddr, maxAddr);
        } else if (l.TrySplit('-', f, s) || l.TrySplit(';', f, s)) {
            return TIpInterval(TAddr(StripString(f)), TAddr(StripString(s).Before(' ').Before('\t')));
        }

        const TAddr ret(StripString(l).Before(' ').Before('\t'));

        return TIpInterval(ret, ret);
    }

    TIpInterval TIpInterval::MakeEmpty() {
        return TIpInterval(TAddr::FromIp(0), TAddr::FromIp(0));
    }

    TString TIpInterval::Print() const {
        return IpBeg.ToString() + "-" + IpEnd.ToString();
    }


} // namespace NAntiRobot
