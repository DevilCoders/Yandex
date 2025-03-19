#pragma once

#include <util/generic/strbuf.h>
#include <library/cpp/string_utils/url/url.h>

// urlhash with reversed dword order, for historical reasons
ui64 UrlHash64Reversed(const char* s, size_t len);
ui64 UrlHash64Reversed(TStringBuf s);

// urlhash as returned by UrlId()
ui64 UrlHash64(TStringBuf s);

inline ui64 UrlHash64FixUrl(TStringBuf s) {
    return UrlHash64(CutHttpPrefix(s, true));
}

inline ui64 PackUrlHash64(ui32 urlHash1, ui32 urlHash2) {
    return (((ui64)urlHash1) << 32) + (ui64)urlHash2;  // note: mixed endian
}

inline void UnPackUrlHash64(ui64 h, ui32& urlHash1, ui32& urlHash2) {
    urlHash2 = (ui32)h;
    urlHash1 = h >> 32;
}
