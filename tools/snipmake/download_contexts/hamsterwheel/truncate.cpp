#include "truncate.h"
#include <library/cpp/charset/codepage.h>
#include <util/charset/utf8.h>

namespace NSnippets
{
    TString TruncateUtf8(const TString& s, size_t maxLen)
    {
        size_t chars = 0;
        const char* p = s.data();
        const char* e = s.data() + s.size();
        const char* trimTo = p;
        while (p < e) {
            wchar32 rune;
            size_t runeLen;
            if (SafeReadUTF8Char(rune, runeLen, (const unsigned char*)p, (const unsigned char*)e) != RECODE_OK) {
                break;
            }
            p += runeLen;
            chars += 1;
            if (chars <= maxLen - 1) {
                trimTo = p;
            }
        }
        if (chars <= maxLen) {
            return s;
        }
        return s.substr(0, trimTo - s.data()) + "\u2026";
    }
}
