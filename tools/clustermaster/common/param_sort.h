#pragma once

#include <util/generic/strbuf.h>

bool NiceLessImpl(const TStringBuf&, const TStringBuf&);


template <typename TStringType>
struct TNiceLess {
    bool operator()(const TStringType& a, const TStringType& b) const {
        return NiceLessImpl(TStringBuf(a), TStringBuf(b));
    }
};
