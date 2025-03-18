
#include <library/cpp/deprecated/split/delim_string_iter.h>

#include "coldiff.h"

#include <util/generic/buffer.h>

void RemCols(size_t ind, size_t col, TDelimStringIter& it,
             const TIncExcFilter<size_t>& filter, TBufferOutput& out) {
    for (; it.Valid(); ++it, ++col) {
        if (filter.Check(col)) {
            if (!out.Buffer().Empty()) {
                out << '\t';
            }
            out << '[' << col << ':' << ind << "]\t" << *it;
        }
    }
}

void ColDiff(TStringBuf v0, TStringBuf v1, const TIncExcFilter<size_t>& filter, TBufferOutput& out) {
    size_t col = 0;
    TDelimStringIter it0(v0, "\t");
    TDelimStringIter it1(v1, "\t");
    while (true) {
        if (!it0.Valid()) {
            RemCols(1, col, it1, filter, out);
            break;
        } else if (!it1.Valid()) {
            RemCols(0, col, it0, filter, out);
            break;
        } else {
            if ((*it0 != *it1) && filter.Check(col)) {
                if (!out.Buffer().Empty()) {
                    out << '\t';
                }
                out << '[' << col << ":01]\t" << *it0 << '\t' << *it1;
            }
            ++col;
            ++it0;
            ++it1;
        }
    }
}
