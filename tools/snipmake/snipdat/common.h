#pragma once

#include <util/generic/strbuf.h>

namespace NSnippets {

template<class TKeyValConsumer>
struct TConvertConsumer {
    TKeyValConsumer* Cons;
    TConvertConsumer(TKeyValConsumer* cons)
      : Cons(cons)
    {
    }
    bool Consume(const char* beg, const char* end, const char*) {
        const auto eqPos = TStringBuf{beg, end}.find('=');
        TStringBuf key;
        TStringBuf val;
        if (eqPos != TStringBuf::npos) {
            key = TStringBuf(beg, eqPos);
            val = TStringBuf(beg + eqPos + 1, end);
        } else {
            key = TStringBuf();
            val = TStringBuf(beg, end);
        }
        return Cons->Consume(key, val);
    }
};

template<int N>
inline bool Equals(const TStringBuf& s, const char (&literal)[N]) {
    return s == TStringBuf{literal, N - 1};
}

}
