#include "crossing.h"

template <>
void Out<NCrossing::TLineToken>(IOutputStream& o, const NCrossing::TLineToken& t) {
    o << t.Line << "\t[" << t.Begin << ", " << t.End << "] {" << t.Words << "}";
}

namespace NCrossing {
    void Expand(TCrossing& result, const TCrossing& tokens) {
        TLineToken src;
        TCrossing ranges;
        ui32 dstLine = Max<ui32>();

        TCrossing::const_iterator i = tokens.begin(), e = tokens.end();
        bool run = true;
        while (run) {
            run = i != e;
            if (!run || src.Line != i->first.Line || dstLine != i->second.Line || src.End < i->first.Begin) {
                // Save all crossing ranges for current source range
                TCrossing::const_iterator p = ranges.begin(), q = ranges.end();
                for (; p != q; ++p)
                    result.push_back(*p);
                if (!run)
                    break;
                // Initialize new source range
                src = i->first;
                ranges.clear();
            }
            src.End = i->first.End;
            dstLine = i->second.Line;

            TCrossing tmp;
            tmp.reserve(ranges.size());
            const ui32 begin = i->first.Begin;

            // Iterate through different destination tokens for one source token
            TCrossing::const_iterator j = ranges.begin(), f = ranges.end();
            while (i != e && src.Line == i->first.Line && dstLine == i->second.Line && begin == i->first.Begin) {
                if (j == f || i->second.End < j->second.End) {
                    // Add new destination token
                    tmp.push_back(*i);
                    ++i;
                } else if (j->second.End < i->second.Begin) {
                    // Save unexpanding range
                    result.push_back(*j);
                    ++j;
                } else {
                    // Expand crossing range
                    tmp.push_back(*j);
                    tmp.back().first.End = i->first.End;
                    tmp.back().first.Words += i->first.Words;
                    tmp.back().second.End = i->second.End;
                    tmp.back().second.Words += i->second.Words;
                    ++i;
                    ++j;
                }
            }
            DoSwap(tmp, ranges);
        }
    }
}
