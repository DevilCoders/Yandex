#include "chunk.h"
#include "scanner.h"

#include <util/charset/wide.h>
#include <util/stream/str.h>

namespace ND2 {
namespace NImpl {

TString TChunkSpan::ToString() const {
    TString res;
    {
        TStringOutput out(res);

        out << "{" << TWtringBuf(Begin, End) << "}.";

        if (HasMeaning<SS_WRD>())
            out << "@.";

        if (HasMeaning<SS_NUM>())
            out << "#.";

        if (HasMeaning<SS_ID>())
            out << "I|";

        if (HasMeaning<SS_YEAR>())
            out << "Y|";

        if (HasMeaning<SS_MONTH>())
            out << "M|";

        if (HasMeaning<SS_DAY>())
            out << "D|";

        if (HasMeaning<SS_TIME>())
            out << "T|";

        if (HasMeaning<SS_JUNK>())
            out << "J|";
    }

    if (res.EndsWith('|') || res.EndsWith('.'))
        res.pop_back();

    return res;
}

TString TChunk::ToString() const {
    TString res;

    {
        TStringOutput out(res);

        if (NoSpans()) {
            out << "[]";
        } else {
            out << "[";

            for (ui32 i = 0; i < SpanCount(); ++i) {
                if (!Spans[i].End || !Spans[i].Begin) {
                    out << "(NULL)";
                } else {
                    out << TWtringBuf(Spans[i].Begin, Spans[i].End);
                }
            }

            out << "] ";

            for (ui32 i = 0; i < SpanCount(); ++i) {
                if (!Spans[i].End || !Spans[i].Begin) {
                    out << "(NULL)";
                } else {
                    out << Spans[i].ToString() << " ";
                }
            }
        }
    }

    if (res.EndsWith(' '))
        res.pop_back();

    return res;
}

}
}
