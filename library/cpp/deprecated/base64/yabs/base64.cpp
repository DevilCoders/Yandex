#include "base64.h"

#include <util/system/sys_alloc.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <string.h>

static int const rfc_encode_table[64] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3',
    '4', '5', '6', '7', '8', '9', '-', '_'};

static int const yabs_encode_table[64] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
    'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
    'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
    'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd',
    'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
    'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
    'u', 'v', 'w', 'x', 'y', 'z', '-', '_'};

static int const rfc_decode_table[128] = {
    -1, -1, -1, -1, -1, -1, -1, -1, // 00
    -1, -1, -1, -1, -1, -1, -1, -1, // 08
    -1, -1, -1, -1, -1, -1, -1, -1, // 10
    -1, -1, -1, -1, -1, -1, -1, -1, // 18
    -1, -1, -1, -1, -1, -1, -1, -1, // 20
    -1, -1, -1, 62, -1, 62, 63, 63, // 28
    52, 53, 54, 55, 56, 57, 58, 59, // 30
    60, 61, -1, -1, -1, -1, -1, -1, // 38
    -1, 0, 1, 2, 3, 4, 5, 6,        // 40
    7, 8, 9, 10, 11, 12, 13, 14,    // 48
    15, 16, 17, 18, 19, 20, 21, 22, // 50
    23, 24, 25, -1, -1, -1, -1, 63, // 58
    -1, 26, 27, 28, 29, 30, 31, 32, // 60
    33, 34, 35, 36, 37, 38, 39, 40, // 68
    41, 42, 43, 44, 45, 46, 47, 48, // 70
    49, 50, 51, -1, -1, -1, -1, -1  // 78
};

static int const yabs_decode_table[128] = {
    -1, -1, -1, -1, -1, -1, -1, -1, // 00
    -1, -1, -1, -1, -1, -1, -1, -1, // 08
    -1, -1, -1, -1, -1, -1, -1, -1, // 10
    -1, -1, -1, -1, -1, -1, -1, -1, // 18
    -1, -1, -1, -1, -1, -1, -1, -1, // 20
    -1, -1, -1, -1, -1, 62, 63, -1, // 28
    0, 1, 2, 3, 4, 5, 6, 7,         // 30
    8, 9, -1, -1, -1, -1, -1, -1,   // 38
    -1, 10, 11, 12, 13, 14, 15, 16, // 40
    17, 18, 19, 20, 21, 22, 23, 24, // 48
    25, 26, 27, 28, 29, 30, 31, 32, // 50
    33, 34, 35, -1, -1, -1, -1, 63, // 58
    -1, 36, 37, 38, 39, 40, 41, 42, // 60
    43, 44, 45, 46, 47, 48, 49, 50, // 68
    51, 52, 53, 54, 55, 56, 57, 58, // 70
    59, 60, 61, -1, -1, -1, -1, -1  // 78
};

#define base64_pad '='

static size_t base64_encode(unsigned char const* buf, size_t buf_size, char* str, size_t str_size, int const* table, bool pad, bool rfc) {
    char* ptr = str;

    char charPad;
    if (pad) {
        charPad = rfc ? base64_pad : table[0];
    }

    size_t sizeBase64DecodedFit = GetYabsSizeBase64DecodedFit(str_size, pad);
    if (buf_size > sizeBase64DecodedFit) {
        buf_size = sizeBase64DecodedFit;
    }

    for (size_t i = 0; i < buf_size; i += 3) {
        unsigned char e;

        e = buf[i] >> 2;
        *ptr++ = table[e];

        e = (buf[i] & 0x03) << 4;

        if (i + 1 >= buf_size) {
            *ptr++ = table[e];
            if (pad) {
                *ptr++ = charPad;
                *ptr++ = charPad;
            }
            break;
        }

        e |= buf[i + 1] >> 4;
        *ptr++ = table[e];

        e = (buf[i + 1] & 0x0f) << 2;

        if (i + 2 >= buf_size) {
            *ptr++ = table[e];
            if (pad) {
                *ptr++ = charPad;
            }
            break;
        }

        e |= buf[i + 2] >> 6;
        *ptr++ = table[e];

        e = buf[i + 2] & 0x3f;
        *ptr++ = table[e];
    }

    return ptr - str;
}

size_t YabsBase64Encode(unsigned char const* buf, size_t buf_size, char* str, size_t str_size) {
    return base64_encode(buf, buf_size, str, str_size, yabs_encode_table, true, false);
}

size_t YabsSpylogBase64Encode(unsigned char const* buf, size_t buf_size, char* str, size_t str_size) {
    return base64_encode(buf, buf_size, str, str_size, rfc_encode_table, false, true);
}

size_t YabsRfcBase64Encode(unsigned char const* buf, size_t buf_size, char* str, size_t str_size) {
    return base64_encode(buf, buf_size, str, str_size, rfc_encode_table, true, true);
}

size_t YabsMarketBase64Encode(unsigned char const* buf, size_t buf_size, char* str, size_t str_size) {
    return base64_encode(buf, buf_size, str, str_size, rfc_encode_table, false, true);
}

static ssize_t base64_decode(char const* str, size_t str_size, unsigned char* buf, size_t buf_size, bool& fail, int const* table) {
    size_t j;
    int e;
    unsigned int c;
    char const* ptr = str;
    char const* str_e = str + str_size;

    memset(buf, 0, buf_size);

    fail = false;

    for (j = 0;; j += 3) {
        unsigned char byteOutput;

        if ((ptr == str_e) || (c = *(ptr++)) == base64_pad) {
            return j;
        }
        if (c >= 128 || (e = table[c]) < 0) {
            fail = true;
            return -1;
        }

        byteOutput = e << 2;

        if ((ptr == str_e) || (c = *(ptr++)) == base64_pad) {
            return j;
        }
        if (c >= 128 || (e = table[c]) < 0) {
            fail = true;
            return -1;
        }

        byteOutput |= e >> 4;

        if (j >= buf_size) {
            fail = true;
            return j;
        }

        buf[j] = byteOutput;

        byteOutput = e << 4;

        if ((ptr == str_e) || (c = *(ptr++)) == base64_pad) {
            return j + 1;
        }
        if (c >= 128 || (e = table[c]) < 0) {
            fail = true;
            return -1;
        }

        byteOutput |= e >> 2;

        if (j + 1 >= buf_size) {
            fail = true;
            return j + 1;
        }

        buf[j + 1] = byteOutput;

        byteOutput = e << 6;

        if ((ptr == str_e) || (c = *(ptr++)) == base64_pad) {
            return j + 2;
        }
        if (c >= 128 || (e = table[c]) < 0) {
            fail = true;
            return -1;
        }

        byteOutput |= e;

        if (j + 2 >= buf_size) {
            fail = true;
            return j + 2;
        }

        buf[j + 2] = byteOutput;
    }

    return j;
}

ssize_t YabsBase64Decode(char const* str, size_t str_size, unsigned char* buf, size_t buf_size) {
    bool fail = false;
    return base64_decode(str, str_size, buf, buf_size, fail, yabs_decode_table);
}

ssize_t YabsBase64Decode(char const* str, size_t str_size, unsigned char* buf, size_t buf_size, bool& fail) {
    return base64_decode(str, str_size, buf, buf_size, fail, yabs_decode_table);
}

ssize_t YabsRfcBase64Decode(char const* str, size_t str_size, unsigned char* buf, size_t buf_size) {
    bool fail = false;
    return base64_decode(str, str_size, buf, buf_size, fail, rfc_decode_table);
}

ssize_t YabsRfcBase64Decode(char const* str, size_t str_size, unsigned char* buf, size_t buf_size, bool& fail) {
    return base64_decode(str, str_size, buf, buf_size, fail, rfc_decode_table);
}

TString YabsRfcBase64Encode(TStringBuf const& buf) {
    TString res;
    size_t const blen = (buf.size() + 5) * 4 / 3;
    res.ReserveAndResize(blen);
    size_t const res_len = YabsRfcBase64Encode(reinterpret_cast<unsigned char const*>(buf.data()), buf.size(), res.begin(), blen);
    res.resize(res_len);
    return res;
}

TString YabsRfcBase64Decode(TStringBuf const& buf) {
    TString res;
    size_t const blen = Base64DecodeBufSize(buf.size());
    res.ReserveAndResize(blen);
    char* res_buf = res.begin();
    ssize_t const res_len = YabsRfcBase64Decode(buf.data(), buf.size(), reinterpret_cast<unsigned char*>(res_buf), blen);
    if (res_len <= 0) {
        return TString();
    }
    res.resize(size_t(res_len));
    return res;
}

TString YabsBase64Encode(TStringBuf const& buf) {
    TString res = TString::Uninitialized(Base64EncodeBufSize(buf.size()));
    size_t const res_len = YabsBase64Encode(reinterpret_cast<unsigned char const*>(buf.data()), buf.size(), res.begin(), res.size());
    res.resize(res_len);
    return res;
}

TString YabsBase64Encode(unsigned char const* buf, size_t buf_len) {
    TString res = TString::Uninitialized(Base64EncodeBufSize(buf_len));
    size_t const res_len = YabsBase64Encode(buf, buf_len, res.begin(), res.size());
    res.resize(res_len);
    return res;
}

TString YabsBase64Decode(TStringBuf const& buf) {
    TString res;
    size_t const blen = Base64DecodeBufSize(buf.size());
    res.ReserveAndResize(blen);
    char* res_buf = res.begin();
    ssize_t const res_len = YabsBase64Decode(buf.data(), buf.size(), reinterpret_cast<unsigned char*>(res_buf), blen);
    if (res_len <= 0) {
        return TString();
    }
    res.remove(size_t(res_len));
    res.remove(res.find('\0'));
    return res;
}
