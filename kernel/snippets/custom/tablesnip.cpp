#include "tablesnip.h"
#include "table_handler.h"
#include "struct_common.h"

#include <kernel/snippets/algo/maxfit.h>
#include <kernel/snippets/archive/markup/markup.h>
#include <kernel/snippets/archive/view/order.h>
#include <kernel/snippets/archive/view/view.h>
#include <kernel/snippets/config/config.h>
#include <kernel/snippets/iface/archive/segments.h>
#include <kernel/snippets/qtree/query.h>
#include <kernel/snippets/sent_match/callback.h>
#include <kernel/snippets/sent_match/extra_attrs.h>
#include <kernel/snippets/sent_match/len_chooser.h>
#include <kernel/snippets/sent_match/length.h>
#include <kernel/snippets/sent_match/retained_info.h>
#include <kernel/snippets/sent_match/sent_match.h>
#include <kernel/snippets/sent_match/single_snip.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/simple_cmp/simple_cmp.h>
#include <kernel/snippets/smartcut/pixel_length.h>
#include <kernel/snippets/title_trigram/title_trigram.h>
#include <kernel/snippets/titles/make_title/util_title.h>

#include <kernel/tarc/iface/tarcface.h>

#include <kernel/segmentator/structs/segment_span.h>

namespace NSnippets {

typedef TVector<NStruct::TRCoord> TRows;

static const char* const TABLE_SNIPPETS = "table_snip";
static const char* const TABLE_DATA = "tableData";
static const size_t MAX_ROWS_CNT = 3;
static const size_t BEST_ROWS_CNT = 2;
static const size_t MAX_COL_CNT = 4;
static const size_t MIN_TBL_ROW_CNT = 3;
static const size_t MAX_ROW_TEXT_LEN = 200;
static const float MAX_LINK_CELLS_PART = 0.85f;
static const size_t MIN_LINK_COL_CNT = 2;
static const size_t MIN_LINK_ROW_CNT = 3;

struct TTableSnipData {
    int TableHeader = -1;
    int RowCount = 0;
    int First = -1;
    int Last = -1;
    bool HeaderRow = false;
    bool HeaderCol = false;

    TString Dump(bool dropStat) {
        TStringStream stream;
        stream << "[{";
        stream << "\"rf\":" << First << ",\"rl\":" << Last
               << ",\"hr\":" << HeaderRow << ",\"hc\":" << HeaderCol;
        if (TableHeader > -1) {
            stream << ",\"th\":" << TableHeader;
        }
        if (!dropStat) {
            stream << ",\"rc\":" << RowCount;
        }
        stream << "}]";
        return stream.Str();
    }
};

struct TTableSnipCtx {
    int TableId = -1;
    int TextPartCnt = 0;
    int TextPartStart = -1;
    int MaxColCnt = 0;
    std::pair<int, int> HeaderWordRange = {-1, -1};
    std::pair<int, int> TextWordRange = {-1, -1};
    float TextPartLen = 0.0f;
    NStruct::TTableInfo TableInfo;
    TRows TableRows;
    TRows BestRows;
    const TReplaceContext& RepCtx;
public:
    explicit TTableSnipCtx(const TReplaceContext& repCtx)
        : RepCtx(repCtx)
    {
    }
    bool HasText() const {
        return TextPartCnt > 0;
    }
    size_t DetectDisplayItemCnt(bool shrink) const {
        if (!shrink) {
            if (HasText()) {
                return MAX_ROWS_CNT - 1 ;
            }
            return MAX_ROWS_CNT;
        }
        return MAX_ROWS_CNT - 1;
    }
};

static inline void FillResByItems(TRows& res, const TVector<const NStructRearrange::TItemInfo*>& items, bool original) {
    for (size_t i = 0; i < items.size() && res.size() < MAX_ROWS_CNT; i++) {
        if (original == items[i]->OriginalItem) {
            res.push_back(std::make_pair(items[i]->ContainerID, items[i]->ItemID));
        }
    }
};

static const TRows FindBestHitRows(const TTableSnipCtx& ctx, const TSentsMatchInfo& smInfo) {
    using namespace NStructRearrange;
    TItemInfos infos;
    infos.reserve(ctx.TableInfo.HitRows.size());
    const bool useWeight = ctx.DetectDisplayItemCnt(ctx.RepCtx.Cfg.ShrinkTableRows()) > 1;
    size_t colCnt = Min(MAX_COL_CNT, ctx.TableInfo.CellSpans.front().size());
    for(size_t i = 0; i < ctx.TableInfo.HitRows.size(); i++) {
        const int rowID = ctx.TableInfo.HitRows[i];
        TSpan hitSpan = ctx.TableInfo.CellSpans[rowID].front();
        hitSpan.Last = ctx.TableInfo.CellSpans[rowID][colCnt - 1].Last;
        // fill item seen pos
        TVector<int> itemSeenPos(smInfo.Query.PosCount(), 0);
        for(int j = hitSpan.First; j <= hitSpan.Last; j++) {
            std::pair<int, int> wordRange = GetWordRange(TSpan(j, j), smInfo.SentsInfo);
            if (CheckSpan(wordRange)) {
                const TVector<int>& sentSeenPos = GetSeenPos(wordRange, smInfo.Query, smInfo);
                for (size_t k = 0; k < itemSeenPos.size(); k++) {
                    itemSeenPos[k] += sentSeenPos[k];
                }
            }
        }
        // detect if original item
        bool origItem = false;
        for(size_t j = 0; j < ctx.TableRows.size(); j++) {
            if (rowID == ctx.TableRows[j].second) {
                origItem = true;
                break;
            }
        }
        infos.push_back(TItemInfo(ctx.TableId, rowID, &smInfo.Query, itemSeenPos, origItem, useWeight));
    }

    // build sorted
    const TVector<const TItemInfo*>& topItems = BuildSorted(infos, smInfo.Query, MAX_ROWS_CNT);

    TRows res;
    res.reserve(MAX_ROWS_CNT);
    // add original hit items
    FillResByItems(res, topItems, true);
    // add best hit items
    FillResByItems(res, topItems, false);
    return res;
}

static bool ContainsItem(const TRows& items, const NStruct::TRCoord item) {
    for (size_t i = 0; i < items.size(); i++) {
        if (items[i].first == item.first && items[i].second == item.second) {
            return true;
        }
    }
    return false;
}

static inline void AddRows(TRows& res, const TRows& rows, size_t maxSz) {
    if (res.size() < maxSz) {
        for (size_t i = 0; i < rows.size() && res.size() < maxSz; i++) {
            if (!ContainsItem(res, rows[i])) {
                res.push_back(rows[i]);
            }
        }
    }
}

static void BuildRows(TTableSnipCtx& ctx, const TSentsMatchInfo& smInfo) {
    const TRows& items = ctx.TableRows;
    size_t newSz = ctx.DetectDisplayItemCnt(ctx.RepCtx.Cfg.ShrinkTableRows());
    TRows res;
    res.reserve(newSz);

    // explicitly add first row
    res.push_back(std::make_pair(ctx.TableId, 0));

    // add table hit items
    const TRows& bestRows = FindBestHitRows(ctx, smInfo);
    AddRows(res, bestRows, newSz);
    //add original hitless items
    AddRows(res, items, newSz);

    // add table hitless items
    if (res.size() < newSz) {
        Sort(res.begin(), res.end(), NStructRearrange::TItemOrderCmp<NStruct::TRCoord>());
        int last = ctx.TableRows.back().second;
        for (int i = last + 1; i < static_cast<int>(ctx.TableInfo.RowCount) && res.size() < newSz; i++) {
            NStruct::TRCoord r(ctx.TableId, i);
            if (!ContainsItem(res, r)) {
                res.push_back(r);
            }
        }

        int first = ctx.TableRows.front().second;
        for (int i = first - 1; i >= 0 && res.size() < newSz; i--) {
            NStruct::TRCoord r(ctx.TableId, i);
            if (!ContainsItem(res, r)) {
                res.push_back(r);
            }
        }
    }

    size_t bcnt = Min(res.size(), BEST_ROWS_CNT);
    ctx.BestRows.reserve(bcnt);
    TRows::const_iterator beg = res.begin();
    ctx.BestRows.insert(ctx.BestRows.end(), beg, beg + bcnt);
    Sort(ctx.BestRows.begin(), ctx.BestRows.end(), NStructRearrange::TItemOrderCmp<NStruct::TRCoord>());
    Sort(res.begin(), res.end(), NStructRearrange::TItemOrderCmp<NStruct::TRCoord>());
    ctx.TableRows.swap(res);
}

static const TVector<TSpan> GetRowSpans(const TTableSnipCtx& ctx) {
    TVector<TSpan> spans;
    spans.reserve(ctx.TableRows.size() + 1);
    // add table header span
    if (ctx.TableInfo.HasHeader) {
        spans.push_back(ctx.TableInfo.HeaderSpan);
    }
    // add selected table rows spans
    for (size_t i = 0; i < ctx.TableRows.size(); i++) {
        TSpan unpSpan = ctx.TableInfo.CellSpans[ctx.TableRows[i].second][0];
        unpSpan.Last = ctx.TableInfo.CellSpans[ctx.TableRows[i].second][ctx.MaxColCnt - 1].Last;
        spans.push_back(unpSpan);
    }
    NSentChecker::Compact(spans);
    return spans;
}

namespace NTableFilters {
    static TSpan AsSpan(const TArchiveZoneSpan& zoneSpan) {
        return TSpan(zoneSpan.SentBeg, zoneSpan.SentEnd);
    }

    class TTableFeatures {
    private:
        size_t SentCount = 0;
        size_t CellCount = 0;
        size_t LinkCellCount = 0;
        size_t RowCnt = 0;
        size_t ColCnt = 0;
        TVector<int> SegSentCount;      // segType -> sent count
        TVector<TVector<size_t> > CellTextLen;
    public:
        TTableFeatures(const NStruct::TTableInfo& tinfo, const TArchiveMarkup& markup, const TSentsOrder& order)
            : RowCnt(tinfo.RowCount)
            , ColCnt(tinfo.CellSpans.front().size())
            , SegSentCount(NSegm::STP_COUNT, 0)
            , CellTextLen()
        {
            // fill seg sent count
            const NSegments::TSegmentsInfo* segInfo = markup.GetSegments();

            const size_t colCnt = tinfo.CellSpans.front().size();
            size_t row = 0;
            size_t cell = 0;

            CellTextLen.reserve(tinfo.RowCount);
            CellTextLen.push_back(TVector<size_t>());
            CellTextLen.back().reserve(colCnt);
            CellTextLen.back().push_back(0);
            TArchiveView v;
            DumpResult(order, v);
            for (size_t i = 0; i < v.Size(); ++i) {
                if (tinfo.Span.Contains(v.Get(i)->SentId)) {
                    NSegments::TSegmentCIt sit = segInfo->GetArchiveSegment(*v.Get(i));
                    NSegm::ESegmentType type = segInfo->GetType(sit);
                    SegSentCount[type]++;
                    SentCount++;

                    if (row < tinfo.CellSpans.size() && v.Get(i)->SentId > tinfo.CellSpans[row].back().Last)
                    {
                        CellTextLen.push_back(TVector<size_t>());
                        CellTextLen.back().reserve(colCnt);
                        CellTextLen.back().push_back(0);
                        row++;
                        cell = 0;
                    }
                    if (row < tinfo.CellSpans.size() && cell < tinfo.CellSpans[row].size() &&
                        v.Get(i)->SentId > tinfo.CellSpans[row][cell].Last)
                    {
                        CellTextLen.back().push_back(0);
                        cell++;
                    }
                    CellTextLen.back().back() += v.Get(i)->Sent.size();
                } else if (v.Get(i)->SentId > tinfo.Span.Last) {
                    break;
                }
            }

            // fill cell link counts
            const TVector<TArchiveZoneSpan>& intSpans = markup.GetArcMarkup(ARC_TEXT).GetZone(AZ_ANCHORINT).Spans;
            const TVector<TArchiveZoneSpan>& extSpans = markup.GetArcMarkup(ARC_TEXT).GetZone(AZ_ANCHOR).Spans;
            size_t curIntSpan = 0;
            size_t curExtSpan = 0;
            for (size_t i = 0; i < tinfo.CellSpans.size(); i++) {
                for (size_t j = 0; j < tinfo.CellSpans[i].size(); j++) {
                    CellCount++;
                    TSpan cellSpan = tinfo.CellSpans[i][j];
                    while(curIntSpan < intSpans.size() && intSpans[curIntSpan].SentEnd < cellSpan.First) {
                        curIntSpan++;
                    }
                    while(curExtSpan < extSpans.size() && extSpans[curExtSpan].SentEnd < cellSpan.First) {
                        curExtSpan++;
                    }
                    bool hasIntLink = curIntSpan < intSpans.size() && cellSpan.Contains(AsSpan(intSpans[curIntSpan]));
                    bool hasExtLink = curExtSpan < extSpans.size() && cellSpan.Contains(AsSpan(extSpans[curExtSpan]));
                    if (hasIntLink || hasExtLink) {
                        LinkCellCount++;
                    }
                }
            }
        }

        bool LinkEveryCell() const {
            return CellCount == LinkCellCount;
        }

        size_t LinkCellCnt() const {
            return LinkCellCount;
        }

        size_t RowCount() const {
            return RowCnt;
        }

        size_t ColCount() const {
            return ColCnt;
        }

        size_t GetSentCount() const {
            return SentCount;
        }

        size_t GetSegSentCount(NSegm::ESegmentType t) const {
            Y_ASSERT(t < SegSentCount.size());
            return SegSentCount[t];
        }

        size_t GetCellTextLen(size_t r, size_t c) const {
            Y_ASSERT(r < CellTextLen.size() && c < CellTextLen[r].size());
            return CellTextLen[r][c];
        }
    };

    static bool ExMenuFilter(const TTableFeatures& tableFeatures) {
        return tableFeatures.GetSentCount() > tableFeatures.GetSegSentCount(NSegm::STP_MENU);
    }

    static bool NavTableFilter(const TTableFeatures& tableFeatures) {
        if (tableFeatures.LinkEveryCell()) {
            return false;
        }
        size_t cellCnt = tableFeatures.RowCount() * tableFeatures.ColCount();
        float cnt = static_cast<float>(tableFeatures.LinkCellCnt()) / static_cast<float>(cellCnt);
        if (cnt >= MAX_LINK_CELLS_PART && tableFeatures.ColCount() > MIN_LINK_COL_CNT && tableFeatures.RowCount() > MIN_LINK_ROW_CNT) {
            return false;
        }
        return true;
    }

    static bool RowCountFilter(const TTableFeatures& tableFeatures) {
        return tableFeatures.RowCount() >= MIN_TBL_ROW_CNT;
    }

    static int TextLenFilter(const TRows& rows, const TTableFeatures& tableFeatures) {
        size_t colCnt = Min(MAX_COL_CNT, tableFeatures.ColCount());
        size_t maxCol = colCnt;
        for (size_t i = 0; i < rows.size(); i++) {
            size_t row = rows[i].second;
            size_t rowLen = 0;
            for (size_t j = 0; j < colCnt; j++) {
                size_t cellLen = tableFeatures.GetCellTextLen(row, j);
                if (rowLen + cellLen > MAX_ROW_TEXT_LEN) {
                    if (j < maxCol) {
                        maxCol = j;
                    }
                    break;
                }
                rowLen += cellLen;
            }
        }
        if (maxCol > 1) {
            return maxCol;
        }
        return -1;
    }

    static int TableFilter(const TRows& rows, const NStruct::TTableInfo& tinfo, const TArchiveMarkup& markup, const TSentsOrder& order) {
        TTableFeatures tableFeatures(tinfo, markup, order);
        if (RowCountFilter(tableFeatures) &&
            NavTableFilter(tableFeatures) &&
            ExMenuFilter(tableFeatures))
        {
            return TextLenFilter(rows, tableFeatures);
        }
        return -1;
    }
}

namespace NTableCellsSelector {
    class TTableSpansInfo {
    private:
        TVector<TVector<std::pair<int, int>>> CellSpans;
    public:
        TTableSpansInfo(const NStruct::TTableInfo& tableInfo, const TSentsMatchInfo& smInfo, const TRows& rows, const int colCnt)
            : CellSpans()
        {
            CellSpans.reserve(rows.size());
            for (int i = 0; i < rows.ysize(); i++) {
                CellSpans.emplace_back();
                CellSpans.back().reserve(colCnt);
                for(int j = 0; j < colCnt; j++) {
                    TSpan origSpan = tableInfo.CellSpans[rows[i].second][j];
                    std::pair<int, int> wordRange = GetWordRange(origSpan, smInfo.SentsInfo);
                    CellSpans.back().push_back(wordRange);
                }
            }
        }

        size_t ColCount() const {
            return CellSpans.front().size();
        }

        size_t RowCount() const {
            return CellSpans.size();
        }

        std::pair<int, int> RowPartSpan(size_t row, size_t endCell) const {
            return {CellSpans[row][0].first, CellSpans[row][endCell].second};
        }

        std::pair<int, int> CellSpan(size_t row, size_t cell) const {
            return CellSpans[row][cell];
        }
    };

    struct TTableCells {
        bool Filled = false;
        bool HeaderRow = false;
        bool HeaderCol = false;
        TVector<std::pair<int, int>> RowSpans;

        TSnip AsSnippet(const TSentsMatchInfo& smInfo) const {
            TSnip res;
            for (size_t i = 0; i < RowSpans.size(); i++) {
                res.Snips.push_back(TSingleSnip(RowSpans[i], smInfo));
            }
            return res;
        }
    };

    class TTableModel {
    private:
        struct TColStat {
            double MinLen = 0;
            double PixLen = 0;
            TVector<int> CellWidths;
        };
        const TTableSnipCtx& Ctx;
        const TTableSpansInfo& TableSpans;
        const TSentsInfo& SentsInfo;
        TPixelLengthCalculator PixelLengthCalculator;
        TVector<TColStat> ColStat;

    private:
        float CalcPixelLength(const std::pair<int, int>& wordSpan) const {
            size_t beginOfs = SentsInfo.WordVal[wordSpan.first].TextBufBegin;
            size_t endOfs = SentsInfo.WordVal[wordSpan.second].TextBufEnd;
            return PixelLengthCalculator.CalcLengthInPixels(beginOfs, endOfs, Ctx.RepCtx.Cfg.GetSnipFontSize());
        }
        float CalcPixelLengthInRows(const std::pair<int, int>& wordSpan, int pixelsInLine) const {
            size_t beginOfs = SentsInfo.WordVal[wordSpan.first].TextBufBegin;
            size_t endOfs = SentsInfo.WordVal[wordSpan.second].TextBufEnd;
            return PixelLengthCalculator.CalcLengthInRows(beginOfs, endOfs, Ctx.RepCtx.Cfg.GetSnipFontSize(), pixelsInLine);
        }

    public:
        TTableModel(const TTableSnipCtx& ctx, const TTableSpansInfo& ti, const TSentsMatchInfo& smInfo)
            : Ctx(ctx)
            , TableSpans(ti)
            , SentsInfo(smInfo.SentsInfo)
            , PixelLengthCalculator(SentsInfo.Text, smInfo.GetMatchedWords())
            , ColStat(TableSpans.ColCount())
        {
            for (size_t i = 0; i < TableSpans.RowCount(); i++) {
                for (size_t j = 0; j < TableSpans.ColCount(); j++) {
                    std::pair<int, int> wordSpan = TableSpans.CellSpan(i, j);
                    for (int w = wordSpan.first; w <= wordSpan.second; w++) {
                        float wlen = CalcPixelLength(std::make_pair(w, w));
                        if (wlen > ColStat[j].MinLen) {
                            ColStat[j].MinLen = wlen;
                        }
                    }
                    float cellLen = CalcPixelLength(wordSpan);
                    if (cellLen > ColStat[j].PixLen) {
                        ColStat[j].PixLen = cellLen;
                    }
                }
            }
            if (ColStat.size() == 1) {
                return;
            }
            int tblWidth = Ctx.RepCtx.Cfg.GetYandexWidth();
            for (size_t i = 1; i < ColStat.size(); i++) {
                double minWidthSum = 0;
                double maxWidthSum = 0;
                for (size_t j = 0; j <= i; j++) {
                    minWidthSum += ColStat[j].MinLen + (20 * (j > 0 ? 1 : 0));
                    maxWidthSum += ColStat[j].PixLen;
                }
                if (minWidthSum > tblWidth) {
                    for (size_t j = 0; j <= i; j++) {
                        ColStat[i].CellWidths.push_back(ColStat[j].MinLen);
                    }
                    continue;
                }
                if (maxWidthSum <= tblWidth) {
                    double inc = (tblWidth - maxWidthSum) / (i + 1);
                    ColStat[i].CellWidths.reserve(i + 1);
                    for (size_t j = 0; j <= i; j++) {
                        ColStat[i].CellWidths.push_back(ColStat[j].PixLen + inc);
                    }
                }
                if (maxWidthSum > tblWidth) {
                    double unfitSpace = tblWidth - minWidthSum;
                    ColStat[i].CellWidths.reserve(i);
                    double sum = 0;
                    for (size_t j = 0; j <= i; j++) {
                        double minLen = ColStat[j].MinLen;
                        double pixLen = ColStat[j].PixLen;
                        double inc = (pixLen / maxWidthSum) * unfitSpace;
                        ColStat[i].CellWidths.push_back(minLen + inc);
                        sum += minLen + inc;
                    }
                    if (sum < tblWidth) {
                        double inc = (tblWidth - sum) / (i + 1);
                        ColStat[i].CellWidths.reserve(i);
                        for (size_t j = 0; j <= i; j++) {
                            ColStat[i].CellWidths[j] += inc;
                        }
                    }
                }
            }
        }

        size_t GetSnipRowCount(int row, int endCell) const {
            const TVector<int>& cellWidths = ColStat[endCell].CellWidths;
            int vsize = 0;
            for(int i = 0; i <= endCell; i++) {
                std::pair<int, int> span = TableSpans.CellSpan(row, i);
                float plen = CalcPixelLength(span);
                int len = 1;
                if (cellWidths[i] - 20 < plen * 1.05) {
                    float lenInRows = CalcPixelLengthInRows(span, cellWidths[i]);
                    len = static_cast<int>(lenInRows * 1.05 + 1.0);
                }
                if (len > vsize) {
                    vsize = len;
                }
            }
            return vsize;
        }
    };

    class TCellSelector {
    private:
        const TTableSnipCtx& Ctx;
        const TSentsMatchInfo& SMInfo;
    public:
        TCellSelector(const TTableSnipCtx& ctx, const TSentsMatchInfo& smInfo)
            : Ctx(ctx)
            , SMInfo(smInfo)
        {}

        TTableCells GetCells(const TRows& rows, int maxRows) {
            TTableSpansInfo tblSpans(Ctx.TableInfo, SMInfo, rows, Ctx.MaxColCnt);
            TTableModel rm(Ctx, tblSpans, SMInfo);
            int endCol = 0;
            int maxCellWordLen = 0;
            int tallRowsCnt = 0;
            for (int col = 1; col < Ctx.MaxColCnt; col++) {
                int vsize = 0;
                for (size_t r = 0; r < tblSpans.RowCount(); r++) {
                    size_t rowHeight = rm.GetSnipRowCount(r, col);
                    vsize += rowHeight;
                    if (rowHeight > 1) {
                        tallRowsCnt++;
                    }
                    std::pair<int, int> cspan = tblSpans.CellSpan(r, col);
                    int wlen = cspan.second - cspan.first + 1;
                    if (wlen > maxCellWordLen) {
                        maxCellWordLen = wlen;
                    }
                }

                if (vsize > maxRows) {
                    break;
                } else {
                    endCol = col;
                }
            }
            // upd maxCellLen with first col
            for (size_t r = 0; r < tblSpans.RowCount(); r++) {
                std::pair<int, int> cspan = tblSpans.CellSpan(r, 0);
                int wlen = cspan.second - cspan.first + 1;
                if (wlen > maxCellWordLen) {
                    maxCellWordLen = wlen;
                }
            }
            TTableCells res;
            if (!Ctx.RepCtx.Cfg.DropTallTables()) {
                tallRowsCnt = 0;
            }
            if (endCol == 0 || (maxCellWordLen < 2 && endCol == 1) || tallRowsCnt > 1) {
                return res;
            }

            // check header column & fill row spans & check hits
            bool hits = false;
            for (size_t i = 0; i < tblSpans.RowCount(); i++) {
                std::pair<int, int> wordSpan = tblSpans.RowPartSpan(i, endCol);
                hits |= SMInfo.MatchesInRange(wordSpan.first, wordSpan.second) > 0;
                res.RowSpans.push_back(wordSpan);
            }

            if (!hits) {
                return res;
            }

            res.HeaderCol = Ctx.TableInfo.HasHeaderColumn;
            res.HeaderRow = Ctx.TableInfo.HasHeaderRow;
            res.Filled = true;
            return res;
        }
    };
}

static void FillTableCtx(TTableSnipCtx& ctx, const TArchiveMarkup& markup,
                         const TTableArcViewer& arcViewer,
                         const TSnip::TSnips& snips)
{
    int num = 0;
    for(TSnip::TSnips::const_iterator it = snips.begin(); it != snips.end(); ++it, num++) {
        TSpan span = GetOrigSentSpan(it->GetWordRange(), it->GetSentsMatchInfo()->SentsInfo);
        TTableArcViewer::TRCoordSpan rowRange = arcViewer.CheckSentSpan(span);
        if (rowRange.first.first == -1) {
            ctx.TextPartStart = num;
            ctx.TextWordRange = it->GetWordRange();
            ctx.TextPartCnt++;
            if (ctx.TextPartCnt == 1) {
                ctx.TextPartLen = ctx.RepCtx.SnipWordSpanLen.CalcLength(*it);
            }
        } else {
            // add items
            for(int i = rowRange.first.second; i <= rowRange.second.second; i++) {
                ctx.TableRows.push_back(std::make_pair(rowRange.first.first, i));
            }
        }
    }

    if (ctx.TextPartCnt > 1 || ctx.TableRows.empty()) {
        return;
    }
    Sort(ctx.TableRows.begin(), ctx.TableRows.end(), NStructRearrange::TItemOrderCmp<NStruct::TRCoord>());
    // check different lists
    if (ctx.TableRows.front().first != ctx.TableRows.back().first) {
        return;
    }
    // merge equal
    ctx.TableRows.erase(Unique(ctx.TableRows.begin(), ctx.TableRows.end()), ctx.TableRows.end());

    // ban long text snippets
    if (ctx.TextPartCnt == 1 && ctx.TextPartLen > 1.9f * ctx.RepCtx.LenCfg.GetRowLen()) {
        return;
    }

    // run filters
    arcViewer.FillTableInfo(ctx.TableRows.front().first, ctx.TableInfo);
    ctx.TableId = ctx.TableRows.front().first;
    BuildRows(ctx, *snips.front().GetSentsMatchInfo());
    ctx.MaxColCnt = NTableFilters::TableFilter(ctx.TableRows, ctx.TableInfo, markup, arcViewer.GetSentsOrder());
    if (ctx.MaxColCnt < 2) {
        ctx.TableId = -1;
    }
}

static bool InTable(const std::pair<int, int>& span, const TSentsMatchInfo& smInfo, const NStruct::TTableInfo& tableInfo) {
    TSpan origSpan = GetOrigSentSpan(span, smInfo.SentsInfo);
    return tableInfo.EffectiveSpan.Contains(origSpan);
}

static bool UpdateSnip(TTableSnipCtx& ctx,
                       TSnip& snip,
                       const TSentsMatchInfo& smInfo,
                       const TSnipTitle* title,
                       NTableCellsSelector::TTableCells& cells,
                       TTableSnipData& snipData)
{
    TSnip::TSnips::iterator it = snip.Snips.begin();
    snipData.RowCount = ctx.TableInfo.RowCount;

    TSnip::TSnips snips;
    bool changed = false;
    int n = 0;

    while (it != snip.Snips.end()) {
        if (InTable(it->GetWordRange(), *it->GetSentsMatchInfo(), ctx.TableInfo)) {
            if (!changed) {
                // add table header
                if (CheckSpan(ctx.HeaderWordRange)) {
                    snips.push_back(TSingleSnip(ctx.HeaderWordRange, smInfo));
                    snipData.TableHeader = n;
                    n++;
                }
                // add table rows
                snipData.First = n;
                for (size_t i = 0; i < cells.RowSpans.size(); i++) {
                    snips.push_back(TSingleSnip(cells.RowSpans[i], smInfo));
                    snips.back().SetSnipType(SPT_TABLE);
                    n++;
                }
                snipData.Last = n - 1;
                // add header info
                snipData.HeaderRow = cells.HeaderRow;
                snipData.HeaderCol = cells.HeaderCol;
            }
            changed = true;
            ++it;
            continue;
        } else {
            static const float maxRowLen = 0.95f * ctx.RepCtx.LenCfg.GetRowLen();
            if (n == 0 && CheckSpan(ctx.HeaderWordRange) && ctx.TextPartLen > maxRowLen) {
                NItemCut::TItemCut itemCut(ctx.RepCtx, maxRowLen);
                const TSentsMatchInfo& itemSMInfo = *it->GetSentsMatchInfo();
                std::pair<int, int> wordSpan = itemCut.Cut(it->GetWordRange(), itemSMInfo, title);
                if (CheckSpan(wordSpan)) {
                    snips.push_back(TSingleSnip(wordSpan, itemSMInfo));
                    snips.back().SetAllowInnerDots(true);
                }
            } else {
                snips.push_back(TSingleSnip(it->GetWordRange(), *it->GetSentsMatchInfo()));
                snips.back().SetAllowInnerDots(true);
            }
        }
        ++n;
        ++it;
    }
    if (changed) {
        snip.Snips = snips;
        return true;
    }
    return false;
}

void PrepareTblHeader(TTableSnipCtx& ctx, const TSentsMatchInfo& tableSMInfo, const TSentsMatchInfo& textSMInfo, const TTitleMatchInfo& tableTitleMInfo) {
    if (ctx.TableInfo.HasHeader) {
        std::pair<int, int> headerWordRange = GetWordRange(ctx.TableInfo.HeaderSpan, tableSMInfo.SentsInfo);
        if (CheckSpan(headerWordRange)) {
            TSingleSnip fragment(headerWordRange, tableSMInfo);
            if (fragment.GetTextBuf().length() < 5) {
                return;
            }
            TSnip snip = NMaxFit::GetTSnippet(ctx.RepCtx.Cfg, ctx.RepCtx.SnipWordSpanLen, fragment, ctx.RepCtx.LenCfg.GetRowLen());
            if (!snip.Snips) {
                return;
            }
            int beg = snip.Snips.front().GetFirstWord();
            int end = snip.Snips.front().GetLastWord();
            if (ctx.RepCtx.Cfg.DropTableHeader() && tableSMInfo.MatchesInRange(beg, end) == 0) {
                return;
            }
            if (ctx.TextPartCnt > 0 && ctx.TextPartStart == 0) {
                if (end - beg + 1 < 3) {
                    return;
                }
                THashSet<size_t> wordSet;
                for (int i = ctx.TextWordRange.first; i <= ctx.TextWordRange.second; i++) {
                    wordSet.insert(textSMInfo.SentsInfo.WordVal[i].Word.Hash);
                }
                size_t origWordCnt = wordSet.size();
                for (int i = beg; i <= end; i++) {
                    wordSet.insert(tableSMInfo.SentsInfo.WordVal[i].Word.Hash);
                }
                if (origWordCnt == wordSet.size()) {
                    return;
                }
            }
            int fsent = tableSMInfo.SentsInfo.WordId2SentId(beg);
            int lsent = tableSMInfo.SentsInfo.WordId2SentId(end);
            int simSum = 0;
            for (int i = fsent; i <= lsent; i++) {
                simSum += static_cast<int>(tableTitleMInfo.GetTitleSimilarity(i) + 0.05);
            }
            if (simSum == (lsent - fsent + 1)) {
                return;
            }
            ctx.HeaderWordRange.first = beg;
            ctx.HeaderWordRange.second = end;
        };
    }
}

static bool BanBroken(const TSnip& snip, const TTableSnipData& snipData) {
    // ban snips without table headers & with text part before table
    if (snipData.TableHeader < 0 && snipData.First > 0) {
        return true;
    }
    // ban snips containing | symbol
    int cnt = 0;
    const wchar16 bar('|');
    for (const TSingleSnip& ssnip : snip.Snips) {
        if (cnt >= snipData.First && cnt <= snipData.Last) {
            if (ssnip.GetTextBuf().Contains(bar)) {
                return true;
            }
        }
        ++cnt;
    }
    return false;
}

void TTableSnipReplacer::DoWork(TReplaceManager* manager) {
    const TReplaceContext& repCtx = manager->GetContext();
    if (repCtx.Snip.Snips.empty()) {
        return;
    }

    TTableSnipCtx ctx(repCtx);
    FillTableCtx(ctx, repCtx.Markup, ArcViewer, repCtx.Snip.Snips);
    if (ctx.TableId == -1) {
        return;
    }

    TRetainedSentsMatchInfo& customSnip = manager->GetCustomSnippets().CreateRetainedInfo();
    BuildSMInfo(NSentChecker::TSpanChecker(GetRowSpans(ctx)), customSnip, repCtx, ArcViewer.GetSentsOrder());

    const TSentsMatchInfo& smInfo = *customSnip.GetSentsMatchInfo();
    TTitleMatchInfo tmInfo;
    tmInfo.Fill(smInfo, &repCtx.SuperNaturalTitle);
    PrepareTblHeader(ctx, smInfo, *repCtx.Snip.Snips.front().GetSentsMatchInfo(), tmInfo);

    NTableCellsSelector::TCellSelector cellSelector(ctx, smInfo);

    int maxRows = 5;
    if (ctx.TextPartCnt > 0) {
        maxRows--;
    }
    NTableCellsSelector::TTableCells cells = cellSelector.GetCells(ctx.TableRows, maxRows);
    NTableCellsSelector::TTableCells trCells = cellSelector.GetCells(ctx.BestRows, maxRows);
    NTableCellsSelector::TTableCells* resCells = &cells;

    if (ctx.TableRows.size() == 3) {
        if (!cells.Filled) {
            resCells = &trCells;
        } else if (trCells.Filled) {
            double bgW = TSimpleSnipCmp::CalcWeight(cells.AsSnippet(smInfo), nullptr, true);
            double smW = TSimpleSnipCmp::CalcWeight(trCells.AsSnippet(smInfo), nullptr, true);
            if (smW > bgW) {
                resCells = &trCells;
            }
        }
    }
    if (!resCells->Filled) {
        return;
    }

    TSnip newSnip = repCtx.Snip;
    TTableSnipData snipData;

    bool success = UpdateSnip(ctx, newSnip, smInfo, &repCtx.SuperNaturalTitle, *resCells, snipData);
    if (success) {
        double worig = TSimpleSnipCmp::CalcWeight(repCtx.Snip, &repCtx.SuperNaturalTitle.GetTitleSnip(), true);
        double wtbl = TSimpleSnipCmp::CalcWeight(newSnip, &repCtx.SuperNaturalTitle.GetTitleSnip(), true);
        if (worig > wtbl) {
            return;
        }
        if (repCtx.Cfg.DropBrokenTables() && BanBroken(newSnip, snipData)) {
            return;
        }
        TReplaceResult result;
        result.UseSnip(newSnip, TABLE_SNIPPETS);
        result.AppendSpecSnipAttr(TABLE_DATA, snipData.Dump(repCtx.Cfg.DropTableStat()));
        manager->Commit(result, MRK_TABLESNIP);
    }
}

}
