#pragma once

#include <kernel/search_types/search_types.h>

#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/system/defaults.h>

namespace NGroupingAttrs {

/* SegmentIterator by ironpeter@ */

struct TSegment {
    ui32 BegDoc;
    ui32 EndDoc;

    TSegment()
        : BegDoc(ui32(-1))
        , EndDoc(ui32(-1))
    {
    }
};


class TCategSegmentIter {
private:
    const TVector<TSegment>* Segments;
    size_t Pointer;
    ui32 CurrDoc;

private:
    void SetEof() {
        Pointer = Segments ? Segments->size() : 0;
        CurrDoc = ui32(-1);
    }

public:
    TCategSegmentIter(const TVector<TSegment> *segments)
        : Segments(segments)
        , Pointer(0)
    {
        if (Segments && Segments->size())
            CurrDoc = (*Segments)[0].BegDoc;
        else
            SetEof();
    }

    void SkipTo(ui32 docid) {
        if (!Segments) {
            return;
        }

        while (Pointer < Segments->size() && (*Segments)[Pointer].EndDoc <= docid) {
            ++Pointer;
        }

        if (Pointer == (*Segments).size()) {
            SetEof();
            return;
        }

        CurrDoc = Max((*Segments)[Pointer].BegDoc, docid);
    }

    void Next() {
        if (!Segments) {
            return;
        }

        Y_ASSERT(Valid());
        Y_ASSERT(CurrDoc >= (*Segments)[Pointer].BegDoc);

        const TSegment& curr = (*Segments)[Pointer];
        if (CurrDoc < curr.EndDoc) {
            ++CurrDoc;
            return;
        }

        ++Pointer;

        if (Pointer < Segments->size()) {
            CurrDoc = (*Segments)[Pointer].BegDoc;
        } else {
            SetEof();
        }
    }

    ui32 Current() const {
        return CurrDoc;
    }

    bool Valid() const {
        return Segments && Pointer < Segments->size();
    }
};

class TCategSegments {
private:
    TVector<TSegment> Segments;
    TSegment* Current;

public:
    TCategSegments()
        : Current(nullptr)
    {
    }

    TCategSegmentIter CreateIterator() const {
        return TCategSegmentIter(&Segments);
    }

    void VisitDoc(ui32 docid) {
        if (Current && docid < Current->EndDoc) {
            return;
        }
        if (!Current || docid != Current->EndDoc) {
            TSegment curr;
            curr.BegDoc = docid;
            curr.EndDoc = docid + 1;
            Segments.push_back(curr);
            Current = &Segments.back();
            return;
        } else {
            ++Current->EndDoc;
        }
    }

    void Shrink() {
        Segments.shrink_to_fit();
        Current = nullptr;
    }
};

class TDocsAttrs;

class TSegments {
private:
    typedef THashMap<TCateg, TCategSegments> TData;
    TData Data;

private:
    void Init(const TDocsAttrs& da, const TVector<ui32>& attrnums);

public:
    TSegments() {} //fake for realtime

    TSegments(const TDocsAttrs& da, const TVector<ui32>& attrnums) {
        Init(da, attrnums);
    }

    TCategSegmentIter GetIterator(TCateg categ) const {
        TData::const_iterator res = Data.find(categ);
        if (res != Data.end()) {
            return res->second.CreateIterator();
        }

        return TCategSegmentIter(nullptr);
    }
};

}
