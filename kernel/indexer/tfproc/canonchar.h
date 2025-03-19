#pragma once

#include <util/charset/unidata.h>

// WideCharToYandex glues some characters to reasonable ASCII codes, e.g. U+2013 = U+002D = '-'.
// This procedure mimics this behaviour for control characters where it is appropriate.
inline wchar16 CanonicalChar(wchar32 a) {
    // if specialChars[t] != 0, WideCharToYandex converts any character of type t to specialChars[t]
    // all exceptions are located in standard ASCII: [{ are Ps_START, ]} are Pe_END, #%&*/@\ are Po_OTHER
    static const char specialChars[64] = {
        0, 0, 0, 0, '-' /*Lm_EXTENDER*/, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 'X' /*Nl_LETTER*/, 0, 0, ' ' /*Zs_SPACE*/, ' ' /*Zs_ZWSPACE*/, '\n' /*Zl_LINE*/, '\n' /*Zp_PARAGRAPH*/,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, '-' /*Pd_DASH*/, '-' /*Pd_HYPHEN*/, '(' /*Ps_START*/, '\"' /*Ps_QUOTE*/, ')' /*Pe_END*/, '\"' /*Pe_QUOTE*/, '\"' /*Pi_QUOTE*/,
        '\"' /*Pf_QUOTE*/, '_' /*Pc_CONNECTOR*/, '*' /*Po_OTHER*/, '\"' /*Po_QUOTE*/, '.' /*Po_TERMINAL*/, '-' /*Po_EXTENDER*/, '-' /*Po_HYPHEN*/, '=' /*Sm_MATH*/,
        '-' /*Sm_MINUS*/, '$' /*Sc_CURRENCY*/, '`' /*Sk_MODIFIER*/, 0, '\'' /*Ps_SINGLE_QUOTE*/, 0, '\'' /*Pi_SINGLE_QUOTE*/, '\'' /*Pf_SINGLE_QUOTE*/,
        '\'' /*Po_SINGLE_QUOTE*/, 0, 0, 0, 0, 0, 0, 0
    };
    if (a < 0x80)
        return a;
    char c = specialChars[NUnicode::CharType(a)];
    return c ? c : a;
}
