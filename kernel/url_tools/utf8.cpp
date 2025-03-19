#include "utf8.h"

#include <util/charset/utf8.h>
#include <util/charset/wide.h>
#include <util/memory/tempbuf.h>

inline void UTF8Advance(const unsigned char*& ptr) {
    ptr += UTF8RuneLen(*ptr);
}

bool UTF8ToWChar32(const unsigned char* src, const unsigned char* end, wchar32* dst) {
    wchar32 rune;
    size_t i = 0;
    while (src < end) {
        if (ReadUTF8CharAndAdvance(rune, src, end) != RECODE_OK)
            return false;
        dst[i++] = rune;
    }
    dst[i] = 0;
    return true;
}

bool UTF8ToWChar32Lowercase(const unsigned char* src, const unsigned char* end, wchar32* dst) {
    wchar32 rune;
    size_t i = 0;
    while (src < end) {
        if (ReadUTF8CharAndAdvance(rune, src, end) != RECODE_OK)
            return false;
        dst[i++] = ToLower(rune);
    }
    dst[i] = 0;
    return true;
}

bool WChar32ToUTF8(const wchar32* src, unsigned char* dst, unsigned char* end) {
    size_t rune_len;
    unsigned char* ptr = dst;
    size_t i = 0;
    while (src[i]) {
        WriteUTF8Char(src[i++], rune_len, ptr);
        ptr += rune_len;
        if (ptr > end - MAX_UTF8_LEN)
            return false;
    }
    *ptr = '\0';
    return true;
}

size_t CaseInsensitiveSubstUTF8(TString& string, const TStringBuf& from /* lowercase */, const TStringBuf& to, bool recursive) {
    TTempArray<wchar32> from32(from.size() + 1);
    if (!UTF8ToWChar32((const unsigned char*)from.begin(), (const unsigned char*)from.end(), from32.Data()))
        return 0;

    const unsigned char* begin = (const unsigned char*)string.begin();
    const unsigned char* end = (const unsigned char*)string.end();
    const unsigned char* matchBegin = begin;
    const unsigned char* current = begin;
    const wchar32* pattern = from32.Data();

    size_t result = 0;
    size_t j = 0;
    wchar32 rune;
    while (current < end) {
        if (ReadUTF8CharAndAdvance(rune, current, end) != RECODE_OK)
            return 0;

        if (ToLower(rune) == pattern[j]) {
            j++;
            if (pattern[j] == 0) {
                size_t matchOff = matchBegin - begin;
                size_t matchLen = current - matchBegin;
                string.replace(matchOff, matchLen, to);
                result++;

                //replace may change the length of a string and invalidate the pointers
                begin = (const unsigned char*)string.begin();
                end = (const unsigned char*)string.end();
                matchBegin = begin + matchOff;
                if (!recursive)
                    matchBegin += to.length();
                current = matchBegin;
            }
        } else {
            UTF8Advance(matchBegin);
            current = matchBegin;
        }
    }
    return result;
}
