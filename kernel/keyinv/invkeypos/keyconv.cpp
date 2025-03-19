#include <library/cpp/unicode/normalization/custom_encoder.h>
#include <library/cpp/charset/recyr.hh>
#include <library/cpp/charset/codepage.h>
#include <util/charset/utf8.h>
#include <util/system/mutex.h>
#include <util/generic/ptr.h>
#include <util/generic/yexception.h>
#include <util/generic/singleton.h>

#include "keychars.h"
#include "keyconv.h"

// to do: use mk_encrec
class TReallyYandexEncoder
{
public:
    TReallyYandexEncoder() {
        Enc.Create(&csYandex);
    }
    char Code(wchar32 ch) const {
        return Enc.Code(ch);
    }

private:
    TCustomEncoder Enc;
};

static const TReallyYandexEncoder& YandexEncoder = Default<TReallyYandexEncoder>();

//! @param buffer       a sequential buffer receiving converted characters
//! @param buflen       size of the buffer, it must not be equal to 0
//! @param text         a text to be converted
//! @param textlen      length of the text
//! @param written      a variable that is assigned to the number of characters copied into the buffer if all symbols
//!                     are convertible to the yandex encoding, if not - it is assigned to 0
//! @return pointer to null terminator - the last character copied into the receiving buffer
char* ConvertUnicodeToYandex(char* buffer, size_t buflen, const wchar16* text, size_t textlen, size_t& written) {
    Y_ASSERT(buffer && buflen && text);

    const size_t len = Min(textlen, buflen - 1);
    const wchar16* ch = text;
    const wchar16* const end = text + len;
    char* out = buffer;
    while (ch != end) {
        Y_ASSERT(ch);
        if (*ch == 0) {
            break;
        }
        char code = YandexEncoder.Code(*ch);
        if (code == 0) {
            break;
        }
        *out++ = code;
        ch++;
    }
    *out = 0;

    written = (ch == end ? len : 0);
    return out;
}

inline void ConvertUnicodeToUTF8(char* buffer, size_t buflen, const wchar16* text, size_t textlen, size_t& nwritten) {
    buffer[0] = UTF8_FIRST_CHAR;
    size_t nread = 0;
    RecodeFromUnicode(CODES_UTF8, text, buffer + 1, textlen, buflen - 2, nread, nwritten);
    nwritten += 1; // adding UTF8_FIRST_CHAR
    buffer[nwritten] = 0;
}

bool TryToConvertUTF8ToYandex(char* buffer, size_t buflen, const char* token, size_t toklen)
{
    Y_ASSERT(buffer && buflen && token);

    const unsigned char* s    = (const unsigned char*)token;
    const unsigned char* send = (const unsigned char*)(token + toklen);
    char* dst = buffer;
    size_t remDstLen = buflen;
    wchar32 rune;
    size_t  runeLen;
    while ((s < send) && (remDstLen > 1)) {
        RECODE_RESULT rr = SafeReadUTF8Char(rune, runeLen, s, send);
        if (rr != RECODE_OK) {
            ythrow yexception() << "TryConvertUTF8ToYandex: bad src string: " <<  TString(token, toklen).data();
        }
        char c = YandexEncoder.Code(rune);
        if (!c) {
            return false;
        }
        *dst++ = c;
        --remDstLen;
        s += runeLen;
    }
    *dst = 0;
    return true;
}

inline const char* ConvertFormUTF8ToKey(char* buffer, size_t buflen, const char* token, size_t toklen) {
    if (TryToConvertUTF8ToYandex(buffer, buflen, token, toklen))
        return buffer;

    buffer[0] = UTF8_FIRST_CHAR;
    if (toklen >= (buflen-1))
        ythrow TKeyConvertException() << "Token is too long to be key: " <<  TString(token, toklen).data();
    strncpy(buffer + 1, token, toklen);
    buffer[toklen + 1] = 0;
    return buffer;
}

// would the given char confuse DecodeKey() if used as the first character of a key?
static bool IsConfusingChar(char c)
{
    return c == ATTR_PREFIX || c == OPEN_ZONE_PREFIX || c == CLOSE_ZONE_PREFIX || c == '?';
}

const char* TFormToKeyConvertor::Convert(const wchar16* token, size_t leng, size_t& written) {
    ConvertUnicodeToYandex(Buffer, BufSize, token, leng, written);
    if (!written || IsConfusingChar(Buffer[0]))
        ConvertUnicodeToUTF8(Buffer, BufSize, token, leng, written);
    return Buffer;
}

const char* TFormToKeyConvertor::ConvertAttrValue(const wchar16* value, size_t len, size_t& written) {
    char* p = ConvertUnicodeToYandex(Buffer, BufSize, value, len, written);
    if (!written) {
        Y_ASSERT(p >= Buffer);
        const size_t n = p - Buffer;
        ConvertUnicodeToUTF8(p, BufSize - n, value + n, len - n, written);
        written += n;
    }
    return Buffer;
}

const char* TFormToKeyConvertor::ConvertUTF8(const char* token, size_t leng) {
    return ConvertFormUTF8ToKey(Buffer, BufSize, token, leng);
}

bool HasExtSymbols(const wchar16* token, size_t leng) {
    const wchar16* end = token + leng;
    while (token != end) {
        Y_ASSERT(token);
        if (*token == 0 || YandexEncoder.Code(*token++) == 0) {
            return true;
        }
    }
    return false;
}

void ConvertFromYandexToUTF8(const TStringBuf& src, TString& dst) {
    if (src.empty()) {
        dst.clear();
    } else if (src[0] == UTF8_FIRST_CHAR && IsUtf(src)) {
        dst.assign(src.SubStr(1));
    } else {
        Recode(CODES_YANDEX, CODES_UTF8, src, dst);
    }
}
