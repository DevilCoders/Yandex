#include <util/charset/wide.h>
#include "wtrutil.h"
////////////////////////////////////////////////////////////////////////////////////////////////////
bool StartsWith(const TUtf16String &word, const TUtf16String &start, size_t pos)
{
    if (pos >= word.size())
        return false;
    size_t n = 0;
    while (n < start.size()) {
        if (word[n + pos] != start[n])
            return false;
        ++n;
    }
    return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
bool EndsWith(const TUtf16String &s1, const TUtf16String &s2)
{
    int diff = s1.size() - s2.size();
    if (diff < 0)
        return false;
    for (size_t n = 0; n < s2.size(); ++n)
        if (s2[n] != s1[diff + n])
            return false;
    return true;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
TUtf16String Wtrim(const TUtf16String& str) {
    wchar16 whitespaces[4];
    whitespaces[0] = ' ';
    whitespaces[1] = '\r';
    whitespaces[2] = '\n';
    whitespaces[3] = 0;

    size_t posBegin = str.find_first_not_of(whitespaces);
    size_t posEnd = str.length();

    for (; posEnd > 0 && (str[posEnd - 1] == ' ' || str[posEnd - 1] == '\n' || str[posEnd - 1] == '\r'); --posEnd)
        ;

    return str.substr(posBegin, posEnd - posBegin);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
bool SafeUTF8ToWide(const TString &utf, TUtf16String *res) {
    try {
        *res = UTF8ToWide(utf);
        return true;
    } catch (...) {
        return false;
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
