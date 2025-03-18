#pragma once

#include <util/generic/string.h>
#include <util/generic/hash.h>
#include <library/cpp/numerator/numerate.h>
#include <library/cpp/html/face/parstypes.h>
#include <library/cpp/html/spec/lextype.h>

namespace NSegutils {

const char * GetConstName(PARSED_TYPE t);
const char * GetConstName(HTLEX_TYPE t);
const char * GetConstName(BREAK_TYPE t);
const char * GetConstName(SPACE_MODE t);
const char * GetConstName(TEXT_WEIGHT t);
const char * GetConstName(ATTR_POS t);
const char * GetConstName(ATTR_TYPE t);
const char * GetConstName(MARKUP_TYPE t);
const char * GetConstName(HTLEX_TYPE t);

inline void CopyCharWithEscape(char *& cursor, const char src, const char esc[], const char symb[], const int len) {
    for (int i = 0; i < len; i++) {
        if (esc[i] == src) {
            *(cursor++) = '\\';
            *(cursor++) = symb[i];
            return;
        }
    }

    *(cursor++) = src;
}

inline char* MakeCstring(const char* const src, const int len) {
    const char escapes[] = "\n\r";
    const char symbols[] = "nr0";

    char* tmp = new char[2 * len + 1]; //escapes + zero
    char* tmp1 = tmp;

    for (const char* tmp0 = src; tmp0 != src + len; tmp0++) {
        CopyCharWithEscape(tmp1, *tmp0, escapes, symbols, 3);
    }

    *tmp1 = 0;

    return tmp;
}

};
