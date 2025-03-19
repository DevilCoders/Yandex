#include "table_handler.h"
#include "struct_handler_common.h"

namespace NSnippets {

struct TTablePart : public TMarkupSpan {
    TMarkupSpan HeaderSpan;
};

typedef TTablePart TTableCell;

struct TTableRow : public TTablePart {
    TVector<TTableCell> Cells;

    bool HeaderRow() const {
        return HeaderSpan.Valid() && HeaderSpan.First == First && HeaderSpan.Last == Last;
    }
};

struct TTable : public TTablePart {
    TVector<TTableRow> Rows;
    TSpan EffectiveSpan;
    bool ValidTable;

    TTable()
        : TTablePart()
        , ValidTable(false)
    {}
};

typedef TVector<TTable> TTables;

static TMarkupSpan GetMatchingHeader(const TMarkupSpan& s, const TVector<TArchiveZoneSpan>& headers) {
    for (size_t i = 0; i < headers.size(); i++) {
        if (headers[i].SentBeg == s.First && headers[i].SentEnd == s.Last) {
            return s;
        }
    }
    return TMarkupSpan();
}

static inline size_t RemoteSpanIdxFwd(const TVector<TArchiveZoneSpan>& spans, int lastSent, size_t initIdx) {
    while (initIdx < spans.size() && spans[initIdx].SentBeg <= lastSent) {
        initIdx++;
    }
    return initIdx;
}

struct TTableArcViewer::TImpl : public TViewerImplBase {
    TTables Tables;

    TImpl()
        : TViewerImplBase()
        , Tables()
    {}
    ~TImpl() override {}

    void OnMarkup(const TArchiveMarkupZones& zones) override {
        BuildTables(zones);
        FillSentOrder();
    }

    // generate data from markup stuff
    void BuildTables(const TArchiveMarkupZones& zones) {
        const TVector<TArchiveZoneSpan>& headers = zones.GetZone(AZ_STRICT_HEADER).Spans;
        const TVector<TArchiveZoneSpan>& headers2 = zones.GetZone(AZ_HEADER).Spans;
        const TVector<TArchiveZoneSpan>& tableSpans = zones.GetZone(AZ_TABLE).Spans;
        const TVector<TArchiveZoneSpan>& tableRowSpans = zones.GetZone(AZ_TABLE_ROW).Spans;
        const TVector<TArchiveZoneSpan>& tableCellSpans = zones.GetZone(AZ_TABLE_CELL).Spans;
        Tables.reserve(tableSpans.size());

        size_t curRow = 0;
        size_t curCell = 0;
        for (size_t i = 0; i < tableSpans.size(); i++) {
            bool hits = false;
            {
                TTable table;
                table.First = tableSpans[i].SentBeg;
                table.Last = tableSpans[i].SentEnd;
                table.EffectiveSpan = table;
                table.HeaderSpan = GetPreviousHeader(table, headers);
                if (table.HeaderSpan.Valid()) {
                    MarkHitsInSpan(table.HeaderSpan, HitSents);
                    table.EffectiveSpan.First = table.HeaderSpan.First;
                }
                Tables.push_back(table);
            }
            TTable& table = Tables.back();

            // detect last span idx
            size_t j = RemoteSpanIdxFwd(tableRowSpans, table.Last, curRow);
            table.Rows.reserve(j - curRow);
            bool validTable = true;
            int cellCnt = -1;
            while (curRow < j) {
                {
                    TTableRow row;
                    row.First = tableRowSpans[curRow].SentBeg;
                    row.Last = tableRowSpans[curRow].SentEnd;
                    row.ContainHits = false;
                    table.Rows.push_back(row);
                }
                TTableRow& row = table.Rows.back();

                // detect last span idx
                size_t k = RemoteSpanIdxFwd(tableCellSpans, row.Last, curCell);
                row.Cells.reserve(k - curCell);

                while (curCell < k) {
                    TTableCell cell;
                    cell.First = tableCellSpans[curCell].SentBeg;
                    cell.Last = tableCellSpans[curCell].SentEnd;
                    cell.HeaderSpan = GetMatchingHeader(cell, headers);
                    MarkHitsInSpan(cell, HitSents);
                    row.HeaderSpan.MergePart(cell.HeaderSpan);
                    row.Cells.push_back(cell);
                    curCell++;
                }
                if (cellCnt == -1) {
                    cellCnt = row.Cells.ysize();
                }
                if (cellCnt != row.Cells.ysize() || cellCnt <= 1) {
                    validTable = false;
                }
                hits |= MarkHitsInSpan(row, HitSents);

                if (!CheckSpanLen(row, 10)) {
                    row.First = 0;
                    table.Last = 0;
                    validTable = false;
                }
                curRow++;
            }
            table.ContainHits = hits;
            table.ValidTable = validTable && table.Rows.size() > 1 && table.Valid();
        }
        // find missing headers: between table and first tr
        for (size_t i = 0; i < Tables.size(); i++) {
            if (Tables[i].ValidTable && Tables[i].Rows.front().First > Tables[i].First) {
                TTable& tbl = Tables[i];
                tbl.First = tbl.Rows.front().First;
                if (!tbl.HeaderSpan.Valid()) {
                    tbl.EffectiveSpan.First = tbl.First;
                    TMarkupSpan hdr = GetPreviousHeader(tbl, headers2);
                    if (hdr.Valid()) {
                        tbl.HeaderSpan = hdr;
                        tbl.EffectiveSpan.First = tbl.HeaderSpan.First;
                    }
                }
            }
        }
    }

    void FillSentOrder() {
        for (size_t i = 0; i < Tables.size(); i++) {
            if (!Tables[i].ValidTable) {
                continue;
            }
            AddHeaderSpan(Tables[i].HeaderSpan, HitOrderGen);
            size_t rowCount = Tables[i].Rows.size();
            for (size_t j = 0; j < rowCount; j++) {
                const TTableRow& row = Tables[i].Rows[j];
                if (CheckSpanLen(row, 10)) {
                    HitOrderGen.PushBack(row.First, row.Last);
                }
            }
        }
    }

    // table stuff
    bool CheckHeaderColumn(const TTable& table) const {
        for (size_t i = 0; i < table.Rows.size(); i++) {
            if (table.Rows[i].Cells.empty() || !table.Rows[i].Cells[0].HeaderSpan.Valid()) {
                return false;
            }
        }
        return true;
    }
    std::pair<NStruct::TRCoord, NStruct::TRCoord> CheckSentSpan(TSpan sentSpan) const {
        for(int i = 0; i < Tables.ysize(); i++) {
            if (Tables[i].EffectiveSpan.Contains(sentSpan)) {
                if (!Tables[i].ValidTable) {
                    continue;
                }
                int firstRow = -1;
                int lastRow = -1;
                for (int j = 0; j < Tables[i].Rows.ysize(); j++) {
                    if (Tables[i].Rows[j].Contains(sentSpan.First)) {
                        firstRow = j;
                    }
                    if (Tables[i].Rows[j].Contains(sentSpan.Last)) {
                        lastRow = j;
                        break;
                    }
                }
                if (firstRow == -1) {
                    firstRow = 0;
                }
                if (firstRow <= lastRow) {
                    return {{i, firstRow}, {i, lastRow}};
                }
            }
        }
        return {NStruct::BAD_PAIR, NStruct::BAD_PAIR};
    }
    void FillTableInfo(ui16 tableNum, NStruct::TTableInfo& tableInfo) const {
        const TTable& table = Tables[tableNum];

        // fill table spans info
        tableInfo.Span = table;
        tableInfo.EffectiveSpan = table.EffectiveSpan;

        if (table.HeaderSpan.Valid()) {
            tableInfo.HasHeader = true;
            tableInfo.HeaderSpan = table.HeaderSpan;
        }

        // header row & col
        tableInfo.HasHeaderRow = table.Rows.size() > 0 && table.Rows[0].HeaderRow();
        tableInfo.HasHeaderColumn = CheckHeaderColumn(table);
        tableInfo.RowCount = table.Rows.size();

        // row & cell spans & hit rows
        tableInfo.CellSpans.reserve(table.Rows.size());
        for (size_t i = 0; i < table.Rows.size(); i++) {
            tableInfo.CellSpans.push_back(TVector<TSpan>());
            tableInfo.CellSpans.back().reserve(table.Rows[i].Cells.size());
            for (size_t j = 0; j < table.Rows[i].Cells.size(); j++) {
                tableInfo.CellSpans.back().push_back(table.Rows[i].Cells[j]);
            }
            if (table.Rows[i].ContainHits) {
                tableInfo.HitRows.push_back(i);
            }
        }
    }
};

TTableArcViewer::TTableArcViewer()
    : Impl(new TTableArcViewer::TImpl())
{}

TTableArcViewer::~TTableArcViewer()
{}

// generic
const TSentsOrder& TTableArcViewer::GetSentsOrder() const {
    return Impl->GetSentsOrder();
}

// IArchiveViewer
void TTableArcViewer::OnUnpacker(TUnpacker* unpacker) {
    Impl->OnUnpacker(unpacker);
}

void TTableArcViewer::OnBeforeSents() {
    Impl->OnBeforeSents();
}

void TTableArcViewer::OnMarkup(const TArchiveMarkupZones& zones) {
    Impl->OnMarkup(zones);
}

void TTableArcViewer::OnHitsAndSegments(const TVector<ui16>& hitSents, const NSegments::TSegmentsInfo*) {
    Impl->OnHits(hitSents);
}

void TTableArcViewer::OnEnd()
{}

// table methods
TTableArcViewer::TRCoordSpan TTableArcViewer::CheckSentSpan(TSpan sentSpan) const {
    return Impl->CheckSentSpan(sentSpan);
}

void TTableArcViewer::FillTableInfo(ui16 tableNum, NStruct::TTableInfo& tableInfo) const {
    return Impl->FillTableInfo(tableNum, tableInfo);
}

}
