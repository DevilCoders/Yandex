#pragma once

#include <util/system/types.h>
#include <sys/types.h>

namespace cookiemy {

class Utils {
public:
    static int cookieBase64Decode(const char* aIn, ui64 aInLen, char* aOut, ui64 aOutSize);
    static void cookieBase64Encode(const unsigned char *s, int length, char *store);
    static ui64 base64Length(ui64 len);
    static ui64 unbase64Length(const char* cookie, ui64 size);
    static i64 parseCookieBlock(const char *buf, ui64 max_size, unsigned int *ids, ui64 ids_size);
};

} // namespace cookiemy
