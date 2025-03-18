#pragma once

#include "wordpos.h"

#include <util/stream/output.h>

template <>
inline void Out<TWordPosition>(IOutputStream& o, const TWordPosition& p) {
    o << '[' << p.Doc() << '.' << p.Break() << '.' << p.Word() << '.' << static_cast<int>(p.GetRelevLevel()) << '.' << p.Form() << ']';
}
