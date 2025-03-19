#pragma once

#include <kernel/snippets/archive/view/order.h>
#include <kernel/snippets/span/span.h>

#include <kernel/tarc/iface/tarcface.h>

#include <util/generic/vector.h>
#include <utility>

namespace NSnippets {
namespace NStruct {
    const std::pair<int, int> BAD_PAIR = {-1, -1};
}

struct TMarkupSpan : public TSpan {
    bool ContainHits;

    TMarkupSpan() : TSpan(), ContainHits(false) {};

    bool Valid() const {
        return First > 0 && Last >= First;
    }
    bool MergePart(const TMarkupSpan& other) {
         if (!other.Valid()) {
             return false;
         }
         if (!Valid()) {
             *this = other;
             return true;
         }

         if (LeftTo(other) || other.LeftTo(*this)) {
             Merge(other);
             ContainHits |= other.ContainHits;
             return true;
         }
         return false;
    }
};

class TViewerImplBase {
protected:
    TUnpacker* Unpacker = nullptr;
    TSentsOrder HitSentOrder;
    bool MarkupUsage = false;
    TVector<ui16> HitSents;
    TSentsOrderGenerator HitOrderGen;
public:
    virtual ~TViewerImplBase() {
    }

    // some generic stuff
    const TSentsOrder& GetSentsOrder() const {
        return HitSentOrder;
    }
    bool UseMarkup() const {
        return MarkupUsage;
    }
    void SetUseMarkup(bool use) {
        MarkupUsage = use;
    }

    void OnHits(const TVector<ui16>& hits)
    {
        HitSents = hits;
    }

    // IArchiveViewer part
    void OnUnpacker(TUnpacker* unpacker) {
        Unpacker = unpacker;
    }
    void OnBeforeSents() {
        HitOrderGen.SortAndMerge();
        HitOrderGen.Complete(&HitSentOrder);
        Unpacker->AddRequest(HitSentOrder);
    }
    virtual void OnMarkup(const TArchiveMarkupZones& zones) = 0;
};

struct TArchiveZoneSpanSentCmp {
    bool operator()(const TArchiveZoneSpan& s1, const TArchiveZoneSpan& s2) const {
        return s1.SentEnd < s2.SentEnd;
    }
};

inline static TMarkupSpan GetPreviousHeader(const TMarkupSpan& s, const TVector<TArchiveZoneSpan>& headers) {
    TArchiveZoneSpan tmps;
    tmps.SentEnd = s.First - 1;
    TVector<TArchiveZoneSpan>::const_iterator it = std::lower_bound(headers.begin(), headers.end(), tmps, TArchiveZoneSpanSentCmp());
    if (it != headers.end() && it->SentEnd + 1 == s.First) {
        TMarkupSpan res;
        res.First = it->SentBeg;
        res.Last = it->SentEnd;
        return res;
    }
    return TMarkupSpan();
}

inline static bool MarkHitsInSpan(TMarkupSpan& span, const TVector<ui16>& hitSents) {
    ui16 b = static_cast<ui16>(span.First);
    TVector<ui16>::const_iterator it = std::lower_bound(hitSents.begin(), hitSents.end(), b);
    return span.ContainHits = (it != hitSents.end() && span.Contains(*it));
}

inline static bool CheckSpanLen(const TMarkupSpan& span, size_t len = 5) {
    return span.Len() <= len;
}

inline static void AddHeaderSpan(const TMarkupSpan& header, TSentsOrderGenerator& hitSentOrder) {
    if (header.Valid() && CheckSpanLen(header)) {
        hitSentOrder.PushBack(header.First, header.Last);
    }
}

}
