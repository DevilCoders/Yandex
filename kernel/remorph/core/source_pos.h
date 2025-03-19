#pragma once

#include <util/generic/ptr.h>
#include <util/stream/output.h>
#include <util/string/cast.h>

namespace NRemorph {

struct TSourcePos: public TSimpleRefCount<TSourcePos> {
    TString FileName;
    size_t BegLine;
    size_t EndLine;
    size_t BegCol;
    size_t EndCol;
    TSourcePos()
        : BegLine(0)
        , EndLine(0)
        , BegCol(0)
        , EndCol(0)
    {
    }
    TSourcePos(const TString& fileName, size_t begLine, size_t endLine, size_t begCol, size_t endCol)
        : FileName(fileName)
        , BegLine(begLine)
        , EndLine(endLine)
        , BegCol(begCol)
        , EndCol(endCol)
    {
    }
    virtual ~TSourcePos() {
    }

    TString ToString() const {
        TString res;
        res.append(FileName).append(':').append(::ToString(BegLine)).append(':').append(::ToString(BegCol))
            .append('-').append(::ToString(EndLine)).append(':').append(::ToString(EndCol));
        return res;
    }
};

typedef TIntrusivePtr<TSourcePos> TSourcePosPtr;

inline TSourcePosPtr operator+(TSourcePosPtr a, TSourcePosPtr b) {
    if (!a || !b)
        return TSourcePosPtr();
    return new TSourcePos(a->FileName, a->BegLine, b->EndLine, a->BegCol, b->EndCol);
}

} // NRemorph

Y_DECLARE_OUT_SPEC(inline, NRemorph::TSourcePos, out, sp) {
    out << sp.ToString();
}

Y_DECLARE_OUT_SPEC(inline, NRemorph::TSourcePosPtr, out, sp) {
    if (!sp) {
        out << ":::-:";
    } else {
        out << sp->ToString();
    }
}
