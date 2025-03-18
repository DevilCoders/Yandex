#include "ar_utils.h"

#include <util/generic/singleton.h>
#include <util/stream/printf.h>
#include <util/string/cast.h>
#include <util/string/vector.h>
#include <util/system/hostname.h>

#include <cstdio>

using namespace NAntiRobot;

namespace {
    static inline ui32 StrToIpImpl(const char* ipStr) {
        ui32 res = 0;
        for (int i = 0; i < 3; ++i) {
            const char* dotPos = strchr(ipStr, '.');
            // invalid ip
            if (dotPos < ipStr || dotPos > ipStr + 3)
                return 0;

            res = (res << 8) + FromStringWithDefault<ui32>(TStringBuf(ipStr, (size_t)(dotPos - ipStr)));
            ipStr = dotPos + 1;
        }

        const char* nonDigitPos = ipStr + 1;
        while (isdigit(*nonDigitPos) && nonDigitPos <= ipStr + 3)
            ++nonDigitPos;

        // invalid ip
        if (nonDigitPos > ipStr + 3)
            return 0;

        res = (res << 8) + FromStringWithDefault<ui32>(TStringBuf(ipStr, (size_t)(nonDigitPos - ipStr)));

        return res;
    }

    struct TShortHostNameHolder {
        inline TShortHostNameHolder() {
            TStringBuf host = HostName();
            TStringBuf temp;
            host.Split('.', host, temp);
            ShortHostName = ToString(host);
        }
        TString ShortHostName;
    };
}

namespace NAntiRobot {
    ui32 StrToIp(const TStringBuf& ip) {
        char buf[20];
        ip.strcpy(buf, sizeof(buf));
        return StrToIpImpl(buf);
    }

    TString IpToStr(ui32 ip) {
        char buf[100];
        snprintf(buf, 90, "%" PRIu32 ".%" PRIu32 ".%" PRIu32 ".%" PRIu32,
            ip >> 24, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);

        return buf;
    }

    void Trace(const char* format, ...) {
        va_list params;
        va_start(params, format);
        Printf(Cerr, format, params);
        va_end(params);
    }

    const TString& ShortHostName() {
        return (Singleton<TShortHostNameHolder>())->ShortHostName;
    }

    TString NiceAddrStr(const TString& addr) {
        const TString INVALID_ADDR = "0.0.0.0";
        return addr.empty() || addr[0] == '(' ? INVALID_ADDR : addr;
    }

    TIpRangeMap<size_t> ParseCustomHashingRules(const TString& customHashingRules) {
        TIpRangeMap<size_t> customHashingMap;
        for (const TString& rule : SplitString(customHashingRules, ",")) {
            TStringBuf network, newSize;
            TStringBuf{rule}.Split('=', network, newSize);
            customHashingMap.Insert({TIpInterval::Parse(network), FromString<size_t>(newSize)});
        }
        customHashingMap.EnsureNoIntersections();
        return customHashingMap;
    }

    TString GetRequesterAddress(const TSocket& socket) {
        NAddr::TOpaqueAddr requesterAddr;
        if (getpeername(socket, requesterAddr.MutableAddr(), requesterAddr.LenPtr()) != 0) {
            requesterAddr = NAddr::TOpaqueAddr();
        }
        return NAddr::PrintHost(requesterAddr);
    }

    void LowerHexEncode(TStringBuf src, char *dst) {
        static constexpr TStringBuf bitsToAlphanum = "0123456789abcdef"_sb;

        for (size_t i = 0; i < src.size(); ++i) {
            const auto b = static_cast<ui8>(src[i]);
            dst[2 * i] = bitsToAlphanum[b >> 4];
            dst[2 * i + 1] = bitsToAlphanum[b & 0x0F];
        }
    }

    TString LowerHexEncode(TStringBuf src) {
        if (src.empty()) {
            return {};
        }

        TString ret;
        ret.resize(src.size() * 2);

        LowerHexEncode(src, &ret[0]);
        return ret;
    }
}
