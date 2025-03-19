#pragma once

#include <util/charset/wide.h>
#include <util/string/split.h>
////////////////////////////////////////////////////////////////////////////////////////////////////
bool StartsWith(const TUtf16String &word, const TUtf16String &start, size_t pos = 0);
bool EndsWith(const TUtf16String &s1, const TUtf16String &s2);
TUtf16String Wtrim(const TUtf16String& str);
////////////////////////////////////////////////////////////////////////////////////////////////////
template <class T>
struct TSimpleWPusher {
    inline bool Consume(wchar16* b, wchar16* d, wchar16*) {
        *d = 0;
        C->push_back(b);

        return true;
    }

    T* C;
};
////////////////////////////////////////////////////////////////////////////////////////////////////
template <class T>
inline void Wsplit(wchar16* buf, wchar16 c, T* res) {
    res->resize(0);
    if (*buf == 0)
        return;

    TCharDelimiter<wchar16> delim(c);
    TSimpleWPusher<T> pusher = {res};

    SplitString(buf, delim, pusher);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
template <class T>
inline TUtf16String Wcombine(const T &container, const TString &delimiter) {
    TUtf16String wdelimiter = UTF8ToWide(delimiter);
    typename T::const_iterator it = container.begin();
    TUtf16String ret;
    for (; it != container.end(); ++it) {
        if (!ret.empty())
            ret += wdelimiter;
        ret += *it;
    }
    return ret;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
bool SafeUTF8ToWide(const TString &utf, TUtf16String *res);
////////////////////////////////////////////////////////////////////////////////////////////////////
template <class T, class TElem>
bool Contains(const T &c, const TElem &e) {
    return std::find(c.begin(), c.end(), e) != c.end();
}
