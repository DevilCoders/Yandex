#include <cstring>

#include "util.h"

namespace cookiemy {

static char base64idx[128] = {
    '\377','\377','\377','\377','\377','\377','\377','\377',
    '\377','\377','\377','\377','\377','\377','\377','\377',
    '\377','\377','\377','\377','\377','\377','\377','\377',
    '\377','\377','\377','\377','\377','\377','\377','\377',
    '\377','\377','\377','\377','\377','\377','\377','\377',
    '\377','\377','\377',    62,'\377','\377','\377',    63,
        52,    53,    54,    55,    56,    57,    58,    59,
        60,    61,'\377','\377','\377','\377','\377','\377',
    '\377',     0,     1,     2,     3,     4,     5,     6,
         7,     8,     9,    10,    11,    12,    13,    14,
        15,    16,    17,    18,    19,    20,    21,    22,
        23,    24,    25,'\377','\377','\377','\377','\377',
    '\377',    26,    27,    28,    29,    30,    31,    32,
        33,    34,    35,    36,    37,    38,    39,    40,
        41,    42,    43,    44,    45,    46,    47,    48,
        49,    50,    51,'\377','\377','\377','\377','\377'
};

static int
isbase64(int a) {
    return ('A' <= a && a <= 'Z') || ('a' <= a && a <= 'z') ||
           ('0' <= a && a <= '9') || a == '+' || a == '/';
}

static char
replaceSpace(char c) {
    if (' ' == c) {
        return '+';
    }
    return c;
}

int
Utils::cookieBase64Decode(const char* aIn, ui64 aInLen, char* aOut, ui64 aOutSize) {

    if (!aIn || !aOut) {
        return -1;
    }
    ui64 inLen = aInLen;
    char* out = aOut;
    ui64 outSize = unbase64Length(aIn, aInLen);
    if (aOutSize < outSize) {
        return -1;
    }

    /* Get four input chars at a time and decode them. Ignore white space
     * chars (CR, LF, SP, HT). If '=' is encountered, terminate input. If
     * a char other than white space, base64 char, or '=' is encountered,
     * flag an input error, but otherwise ignore the char.
     */
    int isErr = 0;
    int isEndSeen = 0;
    int b1, b2, b3;
    int a1, a2, a3, a4;
    ui64 inPos = 0;
    ui64 outPos = 0;
    while (inPos < inLen) {
        a1 = a2 = a3 = a4 = 0;
        while (inPos < inLen) {
            a1 = replaceSpace(aIn[inPos++]) & 0xFF;
            if (isbase64(a1)) {
                break;
            }
            else if (a1 == '=') {
                isEndSeen = 1;
                break;
            }
            else if (a1 != '\r' && a1 != '\n' && a1 != ' ' && a1 != '\t') {
                isErr = 1;
            }
        }
        while (inPos < inLen) {
            a2 = replaceSpace(aIn[inPos++]) & 0xFF;
            if (isbase64(a2)) {
                break;
            }
            else if (a2 == '=') {
                isEndSeen = 1;
                break;
            }
            else if (a2 != '\r' && a2 != '\n' && a2 != ' ' && a2 != '\t') {
                isErr = 1;
            }
        }
        while (inPos < inLen) {
            a3 = replaceSpace(aIn[inPos++]) & 0xFF;
            if (isbase64(a3)) {
                break;
            }
            else if (a3 == '=') {
                isEndSeen = 1;
                break;
            }
            else if (a3 != '\r' && a3 != '\n' && a3 != ' ' && a3 != '\t') {
                isErr = 1;
            }
        }
        while (inPos < inLen) {
            a4 = replaceSpace(aIn[inPos++]) & 0xFF;
            if (isbase64(a4)) {
                break;
            }
            else if (a4 == '=') {
                isEndSeen = 1;
                break;
            }
            else if (a4 != '\r' && a4 != '\n' && a4 != ' ' && a4 != '\t') {
                isErr = 1;
            }
        }
        if (isbase64(a1) && isbase64(a2) && isbase64(a3) && isbase64(a4)) {
            a1 = base64idx[a1] & 0xFF;
            a2 = base64idx[a2] & 0xFF;
            a3 = base64idx[a3] & 0xFF;
            a4 = base64idx[a4] & 0xFF;
            b1 = ((a1 << 2) & 0xFC) | ((a2 >> 4) & 0x03);
            b2 = ((a2 << 4) & 0xF0) | ((a3 >> 2) & 0x0F);
            b3 = ((a3 << 6) & 0xC0) | ( a4       & 0x3F);
            out[outPos++] = char(b1);
            out[outPos++] = char(b2);
            out[outPos++] = char(b3);
        }
        else if (isbase64(a1) && isbase64(a2) && isbase64(a3) && (a4 == '=' || a4 == 0)) {
            a1 = base64idx[a1] & 0xFF;
            a2 = base64idx[a2] & 0xFF;
            a3 = base64idx[a3] & 0xFF;
            b1 = ((a1 << 2) & 0xFC) | ((a2 >> 4) & 0x03);
            b2 = ((a2 << 4) & 0xF0) | ((a3 >> 2) & 0x0F);
            out[outPos++] = char(b1);
            out[outPos++] = char(b2);
            break;
        }
        else if (isbase64(a1) && isbase64(a2) && (a3 == '=' || a3 == 0) && (a4 == '=' || a4 == 0)) {
            a1 = base64idx[a1] & 0xFF;
            a2 = base64idx[a2] & 0xFF;
            b1 = ((a1 << 2) & 0xFC) | ((a2 >> 4) & 0x03);
            out[outPos++] = char(b1);
            break;
        }
        else {
            break;
        }
        if (isEndSeen) {
            break;
        }
    } /* end while loop */

    return (isErr) ? -1 : 0;
}

ui64
Utils::base64Length(ui64 len) {
    return (4 * (((len) + 2) / 3));
}

ui64
Utils::unbase64Length(const char* cookie, ui64 size) {
    char* ch = (char*)memchr(cookie, '=', size);
    ui64 len = (NULL == ch) ? size : ch - cookie;
    return (len + 3) / 4 * 3;
}

void
Utils::cookieBase64Encode(const unsigned char *s, int length, char *store) {
    /* Conversion table.
    char safe[]="\0\0\0\0";
    */
    static char tbl[64] = {
        'A','B','C','D','E','F','G','H',
        'I','J','K','L','M','N','O','P',
        'Q','R','S','T','U','V','W','X',
        'Y','Z','a','b','c','d','e','f',
        'g','h','i','j','k','l','m','n',
        'o','p','q','r','s','t','u','v',
        'w','x','y','z','0','1','2','3',
        '4','5','6','7','8','9','+','/'
    };
    int i;
    unsigned char *p = (unsigned char *)store;

    /* Transform the 3x8 bits to 4x6 bits, as required by base64.  */
    for (i = 0; i < length; i += 3) {
        *p++ = tbl[s[0] >> 2];
        *p++ = tbl[((s[0] & 3) << 4) + (s[1] >> 4)];
        *p++ = tbl[((s[1] & 0xf) << 2) + (s[2] >> 6)];
        *p++ = tbl[s[2] & 0x3f];
        s += 3;
    }
    /* Pad the result if necessary...  */
    if (i == length + 1) {
        *(p - 1) = '=';
    }
    else if (i == length + 2) {
        *(p - 1) = *(p - 2) = '=';
    }
    *p = '\0';
}

static i64 getCookieBlockValue(const char *buf, ui64 size, unsigned int &value) {
    if (size <= 0) {
        return 0;
    }
    ui64 shift = 0;
    value = (unsigned char)buf[shift++];
    if (value < 0x80) {
    }
    else if (value < 0xC0) {
        if (size == shift) {
            return 0;
        }
        value = value*256 + (unsigned char)buf[shift++];
        value &= 0x3FFF;
    }
    else if (value < 0xF0) {
        if (size == shift) {
            return 0;
        }
        value = value*256 + (unsigned char)buf[shift++];
        if (size == shift) {
            return 0;
        }
        value = value*256 + (unsigned char)buf[shift++];
        if (size == shift) {
            return 0;
        }
        value = value*256 + (unsigned char)buf[shift++];
        value &= 0xFFFFFFF;
    }
    else {
        return -1;
    }
    return (i64)shift;
}

i64 Utils::parseCookieBlock(const char *buf, ui64 size, unsigned int *ids, ui64 ids_size) {
    i64 shift = 0;
    for (unsigned int k = 0; k < ids_size; ++k) {
        unsigned int value;
        i64 res = getCookieBlockValue(buf + shift, size - shift, value);
        if (res <= 0) {
            return res;
        }
        shift += res;
        if (ids) {
            ids[k] = value;
        }
    }
    return shift;
}

} // namespace cookiemy
