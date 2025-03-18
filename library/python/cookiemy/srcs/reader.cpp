#include <string.h>

#include "reader.h"
#include "util.h"

namespace cookiemy {

static int
getBlockInternal(unsigned char block_id, const char* buf, ui64 buf_size, unsigned int* ids, ui64 *ids_size) {

    if (buf[0] != 'c') {
        return -1;
    }

    if (NULL == ids) {
        *ids_size = 0;
    }

    const char* it = buf;
    const char* it_end = it + buf_size;

    if (it_end == it) {
        return 0;
    }
    ++it;
    if (it_end == it) {
        return 0;
    }

    while (it < it_end) {
        unsigned char id = *it;
        ++it;
        if (0 == id || it_end == it) {
            return 0;
        }
        unsigned char count = *it;
        ++it;
        if (count > it_end - it) {
            return 0;
        }

        if (block_id == id) {
            if (NULL == ids) {
                *ids_size = count;
            }
            else if (count > *ids_size) {
                count = *ids_size;
            }
            i64 shift = Utils::parseCookieBlock(
                (const char*)it, (ui64)(it_end - it), ids ? &(ids[0]) : NULL, count);
            if (shift < 0) {
                return -1;
            }
            return 0;
        }

        i64 shift = Utils::parseCookieBlock(
                (const char*)it, (ui64)(it_end - it), NULL, count);
        if (shift < 0) {
            return -1;
        }
        it += shift;
    }
    return 0;
}

int
getBlock(unsigned char block_id, const char* buf, ui64 buf_size, unsigned int* ids, ui64 ids_size) {
    if (NULL == ids) {
        return -1;
    }
    return getBlockInternal(block_id, buf, buf_size, ids, &ids_size);
}

int
preParseBlock(const char* cookie, ui64 cookie_size, unsigned char block_id,
         char* buf, ui64 buf_size, ui64 *ids_size) {
    int res = Utils::cookieBase64Decode(cookie, cookie_size, buf, buf_size);
    if (res < 0) {
        return -1;
    }
    return getBlockInternal(block_id, buf, buf_size, NULL, ids_size);
}

ui64
getBufferSize(const char* buf, ui64 cookie_size) {
    return Utils::unbase64Length(buf, cookie_size);
}

} // namespace cookiemy
