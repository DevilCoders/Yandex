#include "log_utils.h"

#include <util/string/cast.h>

namespace NAntiRobot {
    inline char x2c(const char*& x) {
        unsigned char c = 0;

        // note: (x & 0xdf) makes x upper case
        if (!isxdigit((unsigned char)(*x)))
            return c;
        c += (*x >= 'A' ? ((*x & 0xdf) - 'A') + 10 : (*x - '0'));
        ++x;

        c *= 16;
        if (!isxdigit((unsigned char)x[0]))
            return c;
        c += (*x >= 'A' ? ((*x & 0xdf) - 'A') + 10 : (*x - '0'));
        ++x;

        return char(c);
    }

    void AdvancedUrlUnescape(char *to, const TStringBuf& from) {
        if (from.empty()) {
            *to = 0;
            return;
        }

        const char* p = from.begin();
        while (p != from.end()) {
            if (*p == '%' && *(p+1) && isxdigit(*(p+1)) &&
                *(p+2) && isxdigit(*(p+2)))
            {
                ++p;
                *to++ = x2c(p);
            } else if (*p == '\\' && *(p+1) == 'x' &&
                *(p+2) && isxdigit(*(p+2)) && *(p+3) && isxdigit(*(p+3)))
            {
                ++p;
                ++p;
                *to++ = x2c(p);
            } else {
                *to++ = *p++;
            }
        }
        *to = 0;
    }

    static inline ui64 GetRequestTimeMicroSeconds(const TStringBuf& timeStr) {
        if (timeStr.size() <= 10) {
            return FromStringWithDefault<ui64>(timeStr) * 1000000;
        }

        if (timeStr.find('.') != TStringBuf::npos) {
            return static_cast<ui64>(FromStringWithDefault<double>(timeStr) * 1000000);
        }

        if (timeStr.size() >= 16) {
            return FromStringWithDefault<ui64>(timeStr);
        }

        return 0;
    }

    TInstant GetRequestTime(const TStringBuf& timeStr) {
        return TInstant::MicroSeconds(GetRequestTimeMicroSeconds(timeStr));
    }
}
