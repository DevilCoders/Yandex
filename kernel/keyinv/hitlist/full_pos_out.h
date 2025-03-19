#pragma once

#include "full_pos.h"

#include <library/cpp/wordpos/wordpos_out.h>

template <>
inline void Out<TFullPosition>(IOutputStream& o, const TFullPosition& p) {
    o << "TFullPosition:";
    Out<TWordPosition>(o, p.Beg);
    o << ':';
    Out<TWordPosition>(o, p.End);
}
