#pragma once

#include <util/string/subst.h>
#include <util/generic/string.h>

namespace NSnippets {

    static constexpr wchar16 CYRILLIC_CAPITAL_LETTER_IO = 0x0401;
    static constexpr wchar16 CYRILLIC_CAPITAL_LETTER_IE = 0x0415;
    static constexpr wchar16 CYRILLIC_SMALL_LETTER_IO = 0x0451;
    static constexpr wchar16 CYRILLIC_SMALL_LETTER_IE = 0x0435;

    inline static void DeSmallYo(TUtf16String& s) {
        if (s.find(CYRILLIC_SMALL_LETTER_IO) != s.npos) {
            SubstGlobal(s, CYRILLIC_SMALL_LETTER_IO, CYRILLIC_SMALL_LETTER_IE);
        }
    }

    inline static void DeCapitalYo(TUtf16String& s) {
        if (s.find(CYRILLIC_CAPITAL_LETTER_IO) != s.npos) {
            SubstGlobal(s, CYRILLIC_CAPITAL_LETTER_IO, CYRILLIC_CAPITAL_LETTER_IE);
        }
    }

    inline static void DeyoInplace(TUtf16String& s) {
        DeCapitalYo(s);
        DeSmallYo(s);
    }
}
