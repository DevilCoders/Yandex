#include "xml.h"
#include <util/charset/utf8.h>
#include <cstdio>

namespace NSnippets {
    static inline wchar32 Read32BitChar(const char*& ii, const char* end) {
        size_t runeLen = 1;
        wchar32 rune;
        if (SafeReadUTF8Char(rune, runeLen, (const unsigned char*)ii, (const unsigned char*)end) == RECODE_OK) {
            ii += runeLen;
            return rune;
        } else {
            ++ii;
            return 0xFFFD;
        }
    }

    static inline bool IsAllowedXml10Char(wchar32 ch) {
        return ch == 0x09 ||
               ch == 0x0A ||
               ch == 0x0D ||
               0x20 <= ch && ch <= 0x7E ||
               ch == 0x85 ||
               0xA0 <= ch && ch <= 0xD7FF ||
               0xE000 <= ch && ch <= 0xFDCF ||
               0xFDE0 <= ch && ch <= 0xFFFD ||
               0x10000 <= ch && ch <= 0x10FFFF && (ch & 0xFFFE) != 0xFFFE;
    }

    TString EncodeTextForXml10(const TString& str, bool needEscapeTags) {
        TString strout;
        if (!str) {
            return TString();
        }
        const char* ii = str.begin();
        const char* end = str.end();
        while (ii < end) {
            const char* iiStart = ii;
            wchar32 ch = Read32BitChar(ii, end);
            if (!IsAllowedXml10Char(ch) || ch == 0xFFFD) {
                strout.append("\xEF\xBF\xBD", 3); // U+FFFD replacement character
            } else if (ch < 0x20) {
                char buf[16];
                int n = sprintf(buf, "&#%" PRIu32 ";", ch);
                if (n > 0) {
                    strout.append(buf, n);
                }
            } else if (needEscapeTags) {
                switch (ch) {
                    case '\"':
                        strout.append("&quot;", 6);
                        break;
                    case '<':
                        strout.append("&lt;", 4);
                        break;
                    case '>':
                        strout.append("&gt;", 4);
                        break;
                    case '&':
                        strout.append("&amp;", 5);
                        break;
                    case '\'':
                        strout.append("&apos;", 6);
                        break;
                    default:
                        strout.append(iiStart, ii);
                        break;
                }
            } else {
                strout.append(iiStart, ii);
            }
        }
        return strout;
    }
}
