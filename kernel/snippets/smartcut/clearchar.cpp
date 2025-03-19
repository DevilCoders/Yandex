#include "clearchar.h"
#include "char_class.h"

#include <util/charset/unidata.h>
#include <util/memory/tempbuf.h>
#include <util/system/yassert.h>

namespace NSnippets
{
    static const wchar16 BREVE_CHAR = 0x306;
    static const wchar16 FORBIDDEN_CHARS[][2] = {
        { 0x0000, 0x001F }, // c0 controls
        { 0x0080, 0x009F }, // c1 controls
        { 0x0300, 0x036F }, // diacritics
        { 0x202E, 0x202E }, // rtl override mark
        { 0x2153, 0x2183 }, // number forms
        { 0x2190, 0x21F3 }, // arrows
        { 0x2200, 0x22F1 }, // mathematical operators
        { 0x2300, 0x23FF }, // technical symbols
        { 0x2400, 0x243F }, // technical pictures
        { 0x2440, 0x245F }, // optical character recognition
        { 0x2460, 0x24FF }, // enclosed numerics
        { 0x2500, 0x257F }, // box drawing
        { 0x2580, 0x259F }, // block elements
        { 0x25A0, 0x25FF }, // geometric shapes
        { 0x2600, 0x26FF }, // miscellaneous symbols
        { 0x2701, 0x27BF }, // dingbats
        { 0x27C0, 0x27EF }, // Miscellaneous Mathematical Symbols-A
        { 0x27F0, 0x27FF }, // Supplemental Arrows-A
        { 0x2800, 0x28FF }, // Braile patterns
        { 0x2900, 0x297F }, // Supplemental Arrows-B
        { 0x2980, 0x29FF }, // Miscellaneous Mathematival Symbols-B
        { 0x2A00, 0x2AFF }, // Supplemental Mathematical operators
        { 0x2B00, 0x2BFF }, // miscellaneous symbols and arrows
        { 0xE000, 0xF8FF }, // private use area
        { 0xFFF0, 0xFFFF }, // specials
    };

    static const wchar16 SPACE_CHAR = ' ';
    static const wchar16 COMMA_CHAR = ',';
    static const wchar16 QUESTION_CHAR = '?';
    static const wchar16 PERIOD_CHAR = '.';

    static const wchar16 PERMITTED_EDGE_CHARS[][2] = {
        { 0x0023, 0x0024 }, // Number Sign .. Dollar Sign
        { 0x0021, 0x0021 }, // !
        { 0x002B, 0x002B }, // +
        { 0x00A2, 0x00A5 }, // Cent Sign, Pound Sign .. Yen Sign
        { 0x00A7, 0x00A7 }, // Section Sign
        { 0x20A0, 0x20B5 }, // Euro-currency Sign .. Cedi Sign
        { 0x20BD, 0x20BD }, // ruble-currency Sign
        { 0x2116, 0x2116 }, // Numero sign
    };

    bool IsPermittedEdgeChar(wchar16 c)
    {
        for (const auto& charRange : PERMITTED_EDGE_CHARS) {
            if (charRange[0] <= c && c <= charRange[1]) {
                return true;
            }
        }
        return false;
    }

    static bool IsBadChar(const wchar16 c)
    {
        for (const auto& charRange : FORBIDDEN_CHARS) {
            if (charRange[0] <= c && c <= charRange[1]) {
                return true;
            }
        }
        return false;
    }

    static bool IsLeftPermitted(const wchar16 c)
    {
        return IsAlpha(c) ||
               IsNumeric(c) ||
               IsLeftQuoteOrBracket(c) ||
               IsPermittedEdgeChar(c) ||
               c == '<';
    }

    static bool IsRightPermitted(const wchar16 c, bool allowSlash, bool allowBreve)
    {
        return c == QUESTION_CHAR ||
               c == '%' ||
               allowSlash && c == '/' ||
               IsAlpha(c) ||
               IsNumeric(c) ||
               IsRightQuoteOrBracket(c) ||
               IsPermittedEdgeChar(c) ||
               c == '>' ||
               allowBreve && c == BREVE_CHAR;
               ;
    }

    static bool SpaceNeeded(const wchar16* c, const wchar16* e)
    {
        const wchar16* nextChar = c + 1;
        if ((*c == COMMA_CHAR) && nextChar != e && IsNumeric(*nextChar)) {
            return false;
        }
        if ((*c == COMMA_CHAR || IsRightPunct(*c)) && nextChar != e && (IsAlphabetic(*nextChar) || IsNumeric(*nextChar))) {
            return true;
        }
        return false;
    }


    static bool IsBreveException(const wchar16 c, const wchar16* prevChar) {
        if (!prevChar) {
            return false;
        }

        return c == BREVE_CHAR && (*prevChar == u'и' || *prevChar == u'И');
    }

    void ClearChars(TUtf16String& s, bool allowSlash, bool allowBreve)
    {
        TTempBuf buf(s.size() * sizeof(wchar16) * 1.5); // reserve additional space for space extensions
        const wchar16* p = s.begin();
        const wchar16* e = s.end();
        const wchar16* prevChar = nullptr;
        int lastAlNum = -1;
        size_t counter = 0;
        bool left = true;
        int lastSeenPeriod = -1;
        bool periods = false;
        while (p != e) {
            const bool isBadChar = IsBadChar(*p);
            if (!isBadChar || (allowBreve && IsBreveException(*p, prevChar))) {
                // skip at beginning of title
                bool alNum = true;
                if (left) {
                    alNum = IsLeftPermitted(*p);
                } else {
                    alNum = IsRightPermitted(*p, allowSlash, allowBreve);
                }

                if (alNum) {
                    lastAlNum = counter;
                    left = false; // switch to rest of string
                }

                if (lastAlNum >= 0) {
                    buf.Append(p, sizeof(wchar16));
                }

                if (*p == PERIOD_CHAR) {
                    lastSeenPeriod = counter;
                    periods = (p - s.data() >= 2) && p[-1] == PERIOD_CHAR && p[-2] == PERIOD_CHAR;
                }

                // detect glued words & insert space
                if (SpaceNeeded(p, e)) {
                    buf.Append(&SPACE_CHAR, sizeof(wchar16));
                    ++counter;
                }
                ++counter;
                prevChar = p;
            } else {
                prevChar = nullptr;
            }
            ++p;
        }
        if (lastAlNum < 0) {
            s.clear();
            return;
        }
        if (lastSeenPeriod > lastAlNum) {
            buf.Append(&PERIOD_CHAR, sizeof(wchar16));
            if (periods) {
                buf.Append(&PERIOD_CHAR, sizeof(wchar16));
                buf.Append(&PERIOD_CHAR, sizeof(wchar16));
            }
        }
        // how many non-letter chars skip at end of title
        int filled = buf.Filled() - (counter - lastAlNum) * sizeof(wchar16) + 1 * sizeof(wchar16); // +1 because counter grows after lastAlNum remembered
        Y_ASSERT(filled >= 0);
        if (filled >= 0) {
            s.assign(reinterpret_cast<wchar16*>(buf.Data()), filled / sizeof(wchar16));
        }
    }

} // namespace NSnippets
