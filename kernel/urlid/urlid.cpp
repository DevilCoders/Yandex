#include "urlid.h"

#include <util/generic/yexception.h>
#include <util/string/hex.h>

static inline char* Hash2StrImpl(ui32 hash, char* buf) {
    return HexEncode(&hash, sizeof(hash), buf);
}

char* NUrlId::Hash2Str(ui64 hash, char* buf) {
    return Hash2StrImpl(hash & (ui64)0xFFFFFFFF, Hash2StrImpl(hash >> 32, buf));
}

void NUrlId::Str2Hash(const char* buf, size_t len, ui64* to) {
    ui32 buffer[2] = {};

    HexDecode(buf, len, buffer);

    *to = (((ui64)buffer[0]) << 32) + (ui64)buffer[1];
}
