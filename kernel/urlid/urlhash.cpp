#include "urlhash.h"

#include <util/generic/string.h>
#include <library/cpp/digest/old_crc/crc.h>
#include <util/digest/fnv.h>

ui64 UrlHash64Reversed(const char* s, size_t len) {
    return Crc<ui64>(s, len, FnvHash<ui64>(s, len, 0));
}

ui64 UrlHash64Reversed(TStringBuf s) {
    return UrlHash64Reversed(s.data(), s.size());
}

ui64 UrlHash64(TStringBuf s) {
    static const ui64 MASK32 = (((ui64)1) << 32) - 1;
    ui64 h = UrlHash64Reversed(s);
    return (h >> 32) | ((h & MASK32) << 32);
}
