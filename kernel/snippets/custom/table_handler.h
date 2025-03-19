#pragma once

#include <kernel/snippets/archive/unpacker/unpacker.h>
#include <kernel/snippets/iface/archive/viewer.h>
#include <kernel/snippets/span/span.h>

#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <utility>

namespace NSnippets {
namespace NStruct {
    typedef std::pair<int, int> TRCoord;  // table row coord: table.row

    struct TTableInfo {
        bool HasHeader = false;
        TSpan HeaderSpan = TSpan(-1, -1);
        bool HasHeaderRow = false;
        bool HasHeaderColumn = false;
        size_t RowCount = 0;
        TSpan Span = TSpan(-1, -1);
        TSpan EffectiveSpan = TSpan(-1, -1);
        TVector<TVector<TSpan> > CellSpans;
        TVector<size_t> HitRows;
    };
}

class TTableArcViewer : public IArchiveViewer {
public:
    typedef std::pair<NStruct::TRCoord, NStruct::TRCoord> TRCoordSpan;
    TTableArcViewer();
    ~TTableArcViewer() override;

    // generic stuff
    const TSentsOrder& GetSentsOrder() const;

    // IArchiveViewer stuff
    void OnUnpacker(TUnpacker* unpacker) override;
    void OnEnd() override;
    void OnMarkup(const TArchiveMarkupZones& zones) override;
    void OnHitsAndSegments(const TVector<ui16>& hitSents, const NSegments::TSegmentsInfo*) override;
    void OnBeforeSents() override;

    // table info methods
    /**< Maps sentSpan to table rows (check if sent fits in table sent span):
     *   sentSpan.First -> TRCoordSpan.first
     *   sentSpan.Last  -> TRCoordSpan.second
     *   If sentSpan first of last sent not in table or not in valid table span
     *   then (-1, -1) rcoord returned
     */
    TRCoordSpan CheckSentSpan(TSpan sentSpan) const;
    void FillTableInfo(ui16 table, NStruct::TTableInfo& tableInfo) const;
private:
    struct TImpl;
    THolder<TImpl> Impl;
};


}
