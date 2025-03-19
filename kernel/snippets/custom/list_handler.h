#pragma once

#include <kernel/snippets/archive/unpacker/unpacker.h>
#include <kernel/snippets/iface/archive/viewer.h>
#include <kernel/snippets/span/span.h>

#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <utility>

namespace NSnippets {

namespace NStruct {
    typedef std::pair<int, int> TLICoord; // list item coord: list.item

    struct TListInfo {
        bool HasHeader = false;
        TSpan HeaderSpan = TSpan(-1, -1);
        bool HasHeaderHits = false;
        size_t ItemCount = 0;
        TSpan Span = TSpan(-1, -1);
        bool Valid = false;
        TVector<TSpan> ItemSpans;
        TVector<size_t> HitItems;
    };
}

class TListArcViewer : public IArchiveViewer {
public:
    TListArcViewer();
    ~TListArcViewer() override;

    // generic stuff
    const TSentsOrder& GetSentsOrder() const;

    // IArchiveViewer stuff
    void OnUnpacker(TUnpacker* unpacker) override;
    void OnEnd() override;
    void OnHitsAndSegments(const TVector<ui16>& hitSents, const NSegments::TSegmentsInfo*) override;
    void OnMarkup(const TArchiveMarkupZones& zones) override;
    void OnBeforeSents() override;

    // list info methods
    bool HasLists() const;
    bool HasListHeader(ui16 list) const;
    std::pair<NStruct::TLICoord, NStruct::TLICoord> CheckList(const TSpan sentSpan) const;
    void FillListInfo(ui16 list, THolder<NStruct::TListInfo>& info) const;

private:
    struct TImpl;
    THolder<TImpl> Impl;
};
}
