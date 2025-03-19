#include "texthits.h"
#include "table.h"

#include <kernel/snippets/sent_match/meta.h>
#include <kernel/snippets/sent_match/restr.h>
#include <kernel/snippets/sent_match/sent_match.h>
#include <kernel/snippets/sent_match/tsnip.h>

#include <kernel/snippets/archive/unpacker/unpacker.h>
#include <kernel/snippets/archive/view/order.h>
#include <kernel/snippets/archive/view/view.h>
#include <kernel/snippets/config/config.h>
#include <kernel/snippets/sent_info/sent_info.h>
#include <kernel/snippets/util/xml.h>

#include <kernel/tarc/iface/tarcface.h>

#include <util/string/util.h>
#include <util/charset/wide.h>
#include <util/generic/algorithm.h>
#include <util/stream/buffer.h>

#include <array>

namespace NSnippets
{
    static std::pair<size_t, size_t> ComputePageOffset(ui32 pageSize, ui32 page, ui32 &pageCount, size_t rowCount, bool singleLongPage)
    {
        if (singleLongPage) {
            pageCount = 1;
            if (page != 1)
                return {0, 0};
            return {0, rowCount};
        }

        pageCount = rowCount / pageSize;
        if (pageCount * pageSize < rowCount)
            ++pageCount;
        if (page < 1 || page > pageCount)
            return {0, 0};

        size_t startOffset = (page - 1) * pageSize;
        size_t endOffset = (startOffset + pageSize <= rowCount) ? (startOffset + pageSize) : rowCount;
        return {startOffset, endOffset};
    }

    static void WriteEscaped(const TUtf16String& w, size_t startOffset, size_t endOffset, TBufferOutput& out)
    {
        if (startOffset >= endOffset || endOffset > w.size())
            return;
        TWtringBuf slice(w.data() + startOffset, w.data() + endOffset);
        out.Write(EncodeTextForXml10(WideToUTF8(slice)));
    }
    static inline const char* GetSegmentType(EArchiveZone zone)
    {
        const static char* SEGMENT = "segment_";
        const size_t SEGLEN = 8;
        const char* name = ToString(zone);
        if (strncmp(name, SEGMENT, SEGLEN) == 0)
            return name + SEGLEN;
        return nullptr;
    }

    struct PaintMarker
    {
        const char* Tag;
        const char* PreText;
        const char* HtmlStyle;
        TString Attrs;
        int Offset;
        int Seq;
        bool Html;
        bool Opening;

        PaintMarker()
            : Tag(nullptr)
            , PreText(nullptr)
            , HtmlStyle(nullptr)
            , Offset()
            , Seq(0)
            , Html()
            , Opening()
        {
        }

        bool operator < (const PaintMarker& other) const
        {
            return Offset < other.Offset || (Offset == other.Offset && Seq < other.Seq);
        }

        void Render(IOutputStream& out) const
        {
            if (HtmlStyle) {
                out.Write("<span style=\"");
                out.Write(HtmlStyle);
                out.Write("\">");
            }
            const char* opener = Html ? "<" : "&lt;";
            const char* closer = Html ? ">" : "&gt;";
            out.Write(opener);
            if (!Opening)
                out.Write('/');
            out.Write(Tag);
            if (Opening && !!Attrs) {
                out.Write(' ');
                out.Write(Attrs);
            }
            out.Write(closer);
            if (HtmlStyle) {
                out.Write("</span>");
            }
            if (PreText)
                out.Write(PreText);
        }
    };

    static PaintMarker CreateHighlightMarker(int position, bool opening)
    {
        PaintMarker marker;
        marker.Html = true;
        marker.Tag = "b";
        marker.Opening = opening;
        marker.Offset = position;
        // ensure <b></b> are drawn inside zone markers
        marker.Seq = opening ? 1 : -1;
        return marker;
    }

    static PaintMarker CreateColorHighlightMarker(int position, bool opening)
    {
        PaintMarker marker;
        marker.Html = true;
        marker.Tag = "span";
        marker.Opening = opening;
        marker.Offset = position;
        marker.Attrs = "style=\"color:red\"";
        return marker;
    }

    static PaintMarker CreateAnchorOpenMarker(int position, bool isExternalLink)
    {
        PaintMarker marker;
        marker.Html = true;
        marker.Tag = "a href=\"#\" onclick=\"return false;\"";
        marker.PreText = isExternalLink ? "&rArr;" : nullptr;
        marker.Opening = true;
        marker.Offset = position;
        return marker;
    }

    static PaintMarker CreateAnchorCloseMarker(int position)
    {
        PaintMarker marker;
        marker.Html = true;
        marker.Tag = "a";
        marker.Opening = false;
        marker.Offset = position;
        return marker;
    }


    class TSnippetHitsCallback::TImpl: public ISnippetTextDebugHandler
    {
        static const int RESULT_PAGE_SIZE = 250;
        const TSnipInfoReqParams Params;
        TSentsOrder Order;

        struct TZoneBoundary
        {
            int ZoneId;
            ui16 Sent;
            ui16 Offset;
            ui32 ZoneSize;
            int AttrCount;
            int AttrOffset;
            bool IsStart;


            static ui32 GetPos(ui16 sent, ui16 offset)
            {
                ui32 tmp = sent;
                tmp <<= 16;
                tmp += offset;
                return tmp;
            }

            static ui32 GetZoneSize(const TArchiveZoneSpan& span)
            {
                return GetPos(span.SentEnd, span.OffsetEnd)
                       - GetPos(span.SentBeg, span.OffsetBeg);
            }

            TZoneBoundary(int zoneId, const TArchiveZoneSpan& span, bool start)
                : ZoneId(zoneId)
                , Sent(start ? span.SentBeg : span.SentEnd)
                , Offset(start ? span.OffsetBeg : span.OffsetEnd)
                , ZoneSize(GetZoneSize(span))
                , AttrCount(0)
                , AttrOffset(0)
                , IsStart(start)
            {
            }

            bool operator< (const TZoneBoundary& right) const
            {
                ui32 leftPos = GetPos(Sent, Offset);
                ui32 rightPos = GetPos(right.Sent, right.Offset);
                if (leftPos != rightPos)
                    return leftPos < rightPos;
                // desired order is: empty zone opening tags, empty zone closing tags, closing tags, opening tags
                else if ((ZoneSize == 0) != (right.ZoneSize == 0))
                    return ZoneSize < right.ZoneSize;
                else if (IsStart != right.IsStart) {
                    if (ZoneSize == 0)
                        return IsStart > right.IsStart;
                    else
                        return IsStart < right.IsStart;
                }
                else if (ZoneSize == 0)
                    return ZoneId < right.ZoneId;
                else if (ZoneSize != right.ZoneSize)
                    return IsStart ? (ZoneSize > right.ZoneSize) : (ZoneSize < right.ZoneSize);
                else
                    return IsStart ? (ZoneId > right.ZoneId) : (ZoneId < right.ZoneId);
            }
        };

        struct TZoneBoundaryPtrLess
        {
            bool operator() (const TZoneBoundary* left, const TZoneBoundary* right) const {
                return *left < *right;
            }
        };

        typedef TVector<TZoneBoundary> TZoneBounds;

        struct THitWord
        {
            int WordStart; // offset in chars from the beginning of the sentence
            int WordLen;   // length in chars

            THitWord(int wordStart, int wordLen)
                : WordStart(wordStart)
                , WordLen(wordLen)
            {

            }
        };

        struct TExplainRestr
        {
            const char* Type;
        };

        struct TExplainSent
        {
            int SentId;     // archive ID (<0 if not from archive; -1 is meta, -2 is static annotation, etc.)
            int InternalId; // index in TSentsInfo (-1 if not there)
            TUtf16String Sent;
            bool IsParabeg;
            TVector<THitWord> Hits;
            TVector<THitWord> FinalSnipFragments;
            TVector<TExplainRestr> Restrictions;
            TZoneBounds::const_iterator ZonesStart;
            TZoneBounds::const_iterator ZonesEnd;

            TExplainSent()
                : SentId(-1)
                , InternalId(-1)
                , IsParabeg(false)
                , ZonesStart()
                , ZonesEnd()
            {
            }

            TExplainSent(int sentId, const TUtf16String& sent, bool isParabeg)
                : SentId(sentId)
                , InternalId(-1)
                , Sent(sent)
                , IsParabeg(isParabeg)
                , ZonesStart()
                , ZonesEnd()
            {
            }
        };

        typedef std::array<int, AZ_COUNT> TZoneDepths; // zone ID -> (current depth, max depth)

        struct TZoneAttr
        {
            TString Name;
            TUtf16String Value;
        };

        TVector<TExplainSent> Sents;
        TVector<TZoneBounds> ZonesBySent;
        TZoneBounds AllBounds;
        TVector<TZoneAttr> AllAttrs;
        bool IsLinkArchive;
        TUnpacker* Unpacker;

        void CalcZoneChanges(TZoneBounds::const_iterator& boundIter, TVector<const TZoneBoundary*>& foundBounds, TZoneDepths& depths, int nextSentId) const;
        void PrintSentPainted(const TExplainSent& sent, TZoneBounds::const_iterator boundIter, TBufferOutput& out, bool showMarkup) const;
        PaintMarker CreateSimpleZoneMarker(const TZoneBoundary& bound) const;
        bool CreateBeautifulZoneMarker(PaintMarker& marker, const TZoneBoundary& bound, bool withSimpleMarkup) const;
        void RenderSentenceBody(
            const TExplainSent& sent,
            TBufferOutput& out,
            const TZoneDepths& zoneDepths,
            TZoneBounds::const_iterator boundIter) const;
        TString RenderSegmentName(
            const int sentId,
            TZoneDepths& zoneDepths,
            TZoneBounds::const_iterator boundIter) const;
        TString RenderSentName(const TExplainSent& sent) const;
        void RenderSentence(
                const TExplainSent& sent,
                IDebugTable& out,
                TZoneDepths& zoneDepths,
                TZoneBounds::const_iterator boundIter) const;
        void RenderZoneChanges(TVector<const TZoneBoundary*>& foundBounds, IDebugTable& cookie) const;
        bool SeekToArchId(int archId, TVector<TExplainSent>::iterator& currentPos);
        void MarkMatches(TExplainSent& sent, const TSentsMatchInfo& matchInfo, int index);
        void GetSentsFilteredByInfoParams(TVector<const TExplainSent*>& result) const;

    public:
        TImpl(const TSnipInfoReqParams& params);

        bool GetIsLinkArchive()
        {
            return IsLinkArchive;
        }
        int CanSkip(int sentNum);
        void OnSent(const TArchiveSent* sent);
        void OnMarkup(const TArchiveMarkupZones& zones) override;
        void AddHits(const TSentsMatchInfo& matchInfo) override;
        void AddRestrictions(const char* restrType, const IRestr& restr) override;
        void MarkFinalSnippet(const TSnip& finalSnippet) override;
        void GetExplanation(IOutputStream& output) const;
        void OnUnpacker(TUnpacker* unpacker) override {
            unpacker->AddRequest(Order);
            Unpacker = unpacker;
        }

        void OnEnd() override {
            const TArchiveView& all = Unpacker->GetAllUnpacked();
            for (size_t i = 0; i != all.Size(); ++i) {
                OnSent(all.Get(i));
            }
        }
    };


    TSnippetHitsCallback::TImpl::TImpl(const TSnipInfoReqParams& params)
        : Params(params)
        , IsLinkArchive()
        , Unpacker(nullptr)
    {
        if (!Params.UnpackedOnly) {
            Order.PushBack(1, 65536);
        }

        IsLinkArchive = params.IsLinkArchive;
    }

    PaintMarker TSnippetHitsCallback::TImpl::CreateSimpleZoneMarker(const TZoneBoundary& bound) const
    {
        PaintMarker marker;
        marker.Html = false;
        marker.Tag = ToString((EArchiveZone)bound.ZoneId);
        marker.Opening = bound.IsStart;
        marker.Seq = 0;
        marker.Offset = bound.Offset;
        if (bound.AttrCount) {
            TStringStream strOut;
            for (int i = bound.AttrOffset, end = bound.AttrOffset + bound.AttrCount; i < end; ++i) {
                strOut << " " << AllAttrs[i].Name << "=\"" << WideToUTF8(AllAttrs[i].Value) << "\"";
            }
            marker.Attrs = strOut.Str();
        }
        marker.HtmlStyle = "color:#206020"; // me designer
        return marker;
    }

    bool TSnippetHitsCallback::TImpl::CreateBeautifulZoneMarker(PaintMarker& marker, const TImpl::TZoneBoundary& bound, bool withSimpleMarkup) const
    {
        EArchiveZone zone = (EArchiveZone)bound.ZoneId;
        bool opening = bound.IsStart;
        int position = bound.Offset;

        if (GetSegmentType(zone) || zone == AZ_MAIN_CONTENT || zone == AZ_MAIN_HEADER)
            return false;
        else if (zone == AZ_ANCHORINT || zone == AZ_ANCHOR) {
            if (opening)
                marker = CreateAnchorOpenMarker(position, zone == AZ_ANCHOR);
            else
                marker = CreateAnchorCloseMarker(position);
        }
        else if (withSimpleMarkup)
            marker = CreateSimpleZoneMarker(bound);
        else
            return false;

        return true;
    }

    void TSnippetHitsCallback::TImpl::RenderSentenceBody(
            const TExplainSent& sent,
            TBufferOutput& out,
            const TZoneDepths& zoneDepths,
            TZoneBounds::const_iterator boundIter) const
    {
        TVector<PaintMarker> paintMarkers;
        for (TVector<THitWord>::const_iterator ii = sent.Hits.begin(), end = sent.Hits.end(); ii != end; ++ii) {
            paintMarkers.push_back(CreateHighlightMarker(ii->WordStart, true));
            paintMarkers.push_back(CreateHighlightMarker(ii->WordStart + ii->WordLen, false));
        }

        for (TVector<THitWord>::const_iterator ii = sent.FinalSnipFragments.begin(), end = sent.FinalSnipFragments.end(); ii != end; ++ii) {
            paintMarkers.push_back(CreateColorHighlightMarker(ii->WordStart, true));
            paintMarkers.push_back(CreateColorHighlightMarker(ii->WordStart + ii->WordLen, false));
        }

        int inAnchor = zoneDepths[AZ_ANCHOR] + zoneDepths[AZ_ANCHORINT];
        if (inAnchor)
            paintMarkers.push_back(CreateAnchorOpenMarker(0, zoneDepths[AZ_ANCHOR]));

        for (; boundIter != AllBounds.end() && boundIter->Sent == sent.SentId; ++boundIter) {
            const EArchiveZone zoneId = (EArchiveZone)boundIter->ZoneId;
            const bool isStart = boundIter->IsStart;

            if (zoneId == AZ_ANCHOR || zoneId == AZ_ANCHORINT) {
                int inAnchorPrev = inAnchor;
                inAnchor += (isStart ? 1 : -1);
                if (inAnchorPrev != 0 && inAnchor != 0)
                    continue;
            }

            PaintMarker marker;
            if (!CreateBeautifulZoneMarker(marker, *boundIter, Params.ShowMarkup))
                continue;
            paintMarkers.push_back(marker);

        }
        if (inAnchor)
            paintMarkers.push_back(CreateAnchorCloseMarker((int)sent.Sent.size()));
        StableSort(paintMarkers.begin(), paintMarkers.end());

        size_t nohitStart = 0;
        for (TVector<PaintMarker>::const_iterator ii = paintMarkers.begin(), end = paintMarkers.end(); ii != end; ++ii) {
            WriteEscaped(sent.Sent, nohitStart, ii->Offset, out);
            ii->Render(out);
            nohitStart = ii->Offset;
        }

        WriteEscaped(sent.Sent, nohitStart, sent.Sent.size(), out);
    }

    void  TSnippetHitsCallback::TImpl::GetSentsFilteredByInfoParams(TVector<const TExplainSent*>& result) const
    {
        result.reserve(Sents.size());
        for (TVector<TExplainSent>::const_iterator ii = Sents.begin(), end = Sents.end(); ii != end; ++ii) {
            if (!Params.UnpackedOnly || ii->InternalId >= 0)
                result.push_back(&(*ii));
        }
    }

    TString  TSnippetHitsCallback::TImpl::RenderSentName(const TExplainSent& sent) const
    {
        if (sent.SentId == META_ORIGIN_SENT_ID) {
            return "meta";
        }
        else if (sent.InternalId < 0) {
            return "<span style=\"color:darkgray\">" + ToString<int>(sent.SentId) + "</span>";
        }
        return ToString<int>(sent.SentId);
    }

    TString TSnippetHitsCallback::TImpl::RenderSegmentName(
        const int sentId,
        TZoneDepths& zoneDepths,
        TZoneBounds::const_iterator boundIter) const
    {
        std::array<bool, AZ_COUNT> overlapped;
        Copy(zoneDepths.begin(), zoneDepths.end(), overlapped.begin());

        for (TZoneBounds::const_iterator ii = boundIter, end = AllBounds.end(); ii != end && ii->Sent == sentId; ++ii) {
            if (ii->IsStart)
                overlapped[ii->ZoneId] = true;
        }

        const char* segName = "&nbsp;";
        const char* segColor = nullptr;

        for (int i = 0; i < AZ_COUNT; ++i) {
            if (!overlapped[i])
                continue;
            const char* maybeSegName = GetSegmentType((EArchiveZone)i);
            if (maybeSegName)
                segName = maybeSegName;
            if ((EArchiveZone)i == AZ_MAIN_CONTENT)
                segColor = "blue";
            else if ((EArchiveZone)i == AZ_MAIN_HEADER)
                segColor = "darkgreen";
        }
        if (segColor) {
            return Sprintf("<span style=\"width:100%%;border-right:2px solid %s; padding-right: 4px;\">%s</span>", segColor, segName);
        }
        return segName;
    }

    void TSnippetHitsCallback::TImpl::RenderSentence(
        const TExplainSent& sent,
        IDebugTable& out,
        TZoneDepths& zoneDepths,
        TZoneBounds::const_iterator boundIter) const
    {
        TBufferOutput body;

        TString sentName = RenderSentName(sent);
        TString segment = RenderSegmentName(sent.SentId, zoneDepths, boundIter);

        if (sent.IsParabeg && Params.ShowMarkup) {
            body.Write("<span style=\"color:green\">¶</span>"); // Picrow sign
        }
        RenderSentenceBody(sent, body, zoneDepths, boundIter);
        out.AddNonEncodedRow(sentName, TString(body.Buffer().Data(), body.Buffer().Size()), segment);
        const TVector<TExplainRestr>& restrs = sent.Restrictions;

        if (!restrs.empty()) {
            body.Buffer().Reset();
            body.Write("<span style=\"text-align:center\"> ------ ");
            for (TVector<TExplainRestr>::const_iterator ii = restrs.begin(), end = restrs.end(); ii != end; ++ii) {
                body.Write(ii->Type);
                body.Write(" ");
            }
            body.Write("------ </span>");
            out.AddNonEncodedRow("---", TString(body.Buffer().Data(), body.Buffer().Size()), "");
        }
    }

    void TSnippetHitsCallback::TImpl::CalcZoneChanges(
            TZoneBounds::const_iterator& boundIter,
            TVector<const TZoneBoundary*>& foundBounds,
            TZoneDepths& depths,
            int nextSentId) const
    {
        foundBounds.clear();

        if (nextSentId <= 0 || boundIter == AllBounds.end() || boundIter->Sent >= nextSentId)
            return;

        TVector<int> openCounts(AZ_COUNT, 0);

        while (boundIter != AllBounds.end() && boundIter->Sent < nextSentId) {
            int& depth = depths[boundIter->ZoneId];
            depth += boundIter->IsStart ? 1 : -1;
            int& openCount = openCounts[boundIter->ZoneId];
            if (openCount > 0 && !boundIter->IsStart) {
                 for (TVector<const TZoneBoundary*>::reverse_iterator ii = foundBounds.rbegin(), end = foundBounds.rend();
                      ii != end; ++ii)
                 {
                     if ((*ii) == nullptr || (*ii)->ZoneId != boundIter->ZoneId)
                         continue;
                     if (ii == foundBounds.rbegin())
                         foundBounds.pop_back();
                     else
                         *ii = nullptr;
                     break;
                 }
                 --openCount;
            }
            else {
                foundBounds.push_back(boundIter);
                if (boundIter->IsStart)
                    ++openCount;
            }
            ++boundIter;
        }

        TVector<const TZoneBoundary*>::iterator src = foundBounds.begin(), dest = src, end = foundBounds.end();
        for (;src != end;++src)
        {
            if (*src != nullptr) {
                *dest = *src;
                ++dest;
            }
        }
        foundBounds.erase(dest, foundBounds.end());
    }

    void TSnippetHitsCallback::TImpl::RenderZoneChanges(TVector<const TZoneBoundary*>& foundBounds, IDebugTable& table) const
    {
        if (!Params.ShowMarkup)
            return;
        if (foundBounds.empty())
            return;

        TBufferOutput out;
        for (TVector<const TZoneBoundary*>::const_iterator ii = foundBounds.begin(), end = foundBounds.end(); ii != end; ++ii)
        {
            EArchiveZone zoneId = (EArchiveZone)((*ii)->ZoneId);
            if (GetSegmentType(zoneId) != nullptr || zoneId == AZ_MAIN_CONTENT || zoneId == AZ_MAIN_HEADER)
                continue;
            PaintMarker marker;
            marker.Html = false;
            marker.Tag = ToString(zoneId);
            marker.Opening = (*ii)->IsStart;
            marker.Render(out);
        }
        if (out.Buffer().Size())
            table.AddNonEncodedRow("...", TString(out.Buffer().Data(), out.Buffer().Size()), "");
    }

    void TSnippetHitsCallback::TImpl::GetExplanation(IOutputStream& output) const
    {
        TBufferOutput formatterOutput; // now that's really stupid
        THolder<IDebugFormatter> formatter;
        if (Params.TableType == INFO_TABSEP)
            formatter.Reset(new TTabDebugFormatter(formatterOutput));
        else if (Params.TableType == INFO_JSON)
            formatter.Reset(new TSerpDebugFormatter(true, formatterOutput));
        else
            formatter.Reset(new TSerpDebugFormatter(false, formatterOutput));

        ui32 pageCount;

        TVector<const TExplainSent*> filteredSents;
        GetSentsFilteredByInfoParams(filteredSents);
        std::pair<size_t, size_t> page = ComputePageOffset(RESULT_PAGE_SIZE, Params.PageNumber, pageCount, filteredSents.size(), Params.Unpaged);
        formatter->SetPageNumber(Params.PageNumber);
        formatter->SetPageCount(pageCount);
        IDebugTable* table = formatter->AddDataTable("SnippetHits");

        TZoneBounds::const_iterator boundIter = AllBounds.begin();
        TVector<const TZoneBoundary*> zoneChanges;
        TZoneDepths depths;
        depths.fill(0);

        for (size_t i = page.first; i < page.second; ++i) {
            const TExplainSent* sent = filteredSents[i];
            CalcZoneChanges(boundIter, zoneChanges, depths, sent->SentId);
            RenderZoneChanges(zoneChanges, *table);
            RenderSentence(*sent, *table, depths, boundIter);
            CalcZoneChanges(boundIter, zoneChanges, depths, sent->SentId + 1);
        }
        CalcZoneChanges(boundIter, zoneChanges, depths, Max<int>());
        RenderZoneChanges(zoneChanges, *table);

        formatter->Finalize();
        output.Write(formatterOutput.Buffer().Data(), formatterOutput.Buffer().Size());
    }

    void TSnippetHitsCallback::TImpl::OnSent(const TArchiveSent* sent) {
        Sents.push_back(TExplainSent(sent->SentId, TUtf16String(sent->Sent.data(), sent->Sent.size()), sent->IsParaStart));
    }

    void TSnippetHitsCallback::TImpl::OnMarkup(const TArchiveMarkupZones& zones)
    {
        for (ui32 zoneId = 0; zoneId < AZ_COUNT; ++zoneId) {
            const TArchiveZone& zone = zones.GetZone(zoneId);
            const TArchiveZoneAttrs& attrs = zones.GetZoneAttrs(zoneId);
            for (TVector<TArchiveZoneSpan>::const_iterator ii = zone.Spans.begin(), end = zone.Spans.end(); ii != end; ++ii) {
                AllBounds.push_back(TZoneBoundary(zoneId, *ii, false));
                AllBounds.push_back(TZoneBoundary(zoneId, *ii, true));
                TZoneBoundary& bound = AllBounds.back();
                bound.AttrOffset = AllAttrs.ysize();
                const THashMap<TString, TUtf16String>* zoneAttrs = attrs.GetSpanAttrs(*ii).AttrsHash;
                if (zoneAttrs == nullptr)
                    continue;
                for (THashMap<TString, TUtf16String>::const_iterator jj = zoneAttrs->begin(); jj != zoneAttrs->end(); ++jj) {
                    TZoneAttr zoneAttr = {jj->first, jj->second};
                    AllAttrs.push_back(zoneAttr);
                    ++bound.AttrCount;
                }
            }
        }

        Sort(AllBounds.begin(), AllBounds.end());
    }

    bool TSnippetHitsCallback::TImpl::SeekToArchId(int archId, TVector<TExplainSent>::iterator& currentPos)
    {
        while (currentPos != Sents.end() && currentPos->SentId < archId) {
            ++currentPos;
        }
        return !(currentPos == Sents.end() || currentPos->SentId != archId);
    }

    void TSnippetHitsCallback::TImpl::MarkMatches(TExplainSent& sent, const TSentsMatchInfo& matchInfo, int index)
    {
        const TSentsInfo& si = matchInfo.SentsInfo;
        TWtringBuf sentBuf = si.GetSentBuf(index);
        int firstWordId = si.FirstWordIdInSent(index);
        int lastWordId = si.LastWordIdInSent(index);
        for (int w = firstWordId; w <= lastWordId; ++w)
            if (matchInfo.IsMatch(w)) {
                TWtringBuf wordBuf = si.GetWordBuf(w);
                sent.Hits.push_back(THitWord(wordBuf.data() - sentBuf.data(), wordBuf.size()));
            }
    }

    void TSnippetHitsCallback::TImpl::AddHits(const TSentsMatchInfo& matchInfo)
    {
        const TSentsInfo& info = matchInfo.SentsInfo;

        TVector<TExplainSent>::iterator sent = Sents.begin();
        TVector<TExplainSent> metaStuff;

        for (int i = 0; i < info.SentencesCount(); ++i) {
            if (info.GetOrigSentId(i) < 0) {
                metaStuff.emplace_back();
                TExplainSent& metaSent = metaStuff.back();
                metaSent.InternalId = i;
                metaSent.SentId = info.GetOrigSentId(i);
                metaSent.Sent = ToWtring(matchInfo.SentsInfo.GetSentBuf(i));
                MarkMatches(metaSent, matchInfo, i);
                continue;
            }
            const TArchiveSent& arcSent = info.GetArchiveSent(i);
            if (arcSent.SourceArc != ARC_TEXT && arcSent.SourceArc != ARC_LINK)
                continue;
            if (!SeekToArchId(arcSent.SentId, sent))
                continue;
            sent->InternalId = i;
            MarkMatches(*sent, matchInfo, i);
        }

        Sents.insert(Sents.begin(), metaStuff.begin(), metaStuff.end());
    }

    void TSnippetHitsCallback::TImpl::AddRestrictions(const char* restrType, const IRestr& restr)
    {
        for (TVector<TExplainSent>::iterator ii = Sents.begin(), end = Sents.end(); ii != end; ++ii) {
            if (ii->InternalId >= 0 && restr(ii->InternalId)) {
                TExplainRestr er;
                er.Type = restrType;
                ii->Restrictions.push_back(er);
            }
        }
    }

    void TSnippetHitsCallback::TImpl::MarkFinalSnippet(const TSnip& finalSnippet)
    {
        TVector<TExplainSent>::iterator sent = Sents.begin();
        for (const TSingleSnip& ssnip : finalSnippet.Snips) {
            const TSentsInfo& si = ssnip.GetSentsMatchInfo()->SentsInfo;
            for (int sentId = ssnip.GetFirstSent(); sentId <= ssnip.GetLastSent(); ++sentId) {
                if (SeekToArchId(si.GetOrigSentId(sentId), sent)) {
                    int w0 = Max(si.FirstWordIdInSent(sentId), ssnip.GetFirstWord());
                    int w1 = Min(si.LastWordIdInSent(sentId), ssnip.GetLastWord());
                    TWtringBuf fragment = si.GetTextBuf(w0, w1);
                    TWtringBuf sentBuf = si.GetSentBuf(sentId);
                    sent->FinalSnipFragments.push_back(THitWord(fragment.data() - sentBuf.data(), fragment.size()));
                }
            }
        }
    }

    TSnippetHitsCallback::TSnippetHitsCallback(const TSnipInfoReqParams& params)
        : Impl(new TImpl(params))
    {
    }

    ISnippetTextDebugHandler* TSnippetHitsCallback::GetTextHandler(bool isByLink)
    {
        if (isByLink == Impl->GetIsLinkArchive()) {
            return Impl.Get();
        } else {
            return nullptr;
        }
    }

    void TSnippetHitsCallback::GetExplanation(IOutputStream& result) const
    {
        Impl->GetExplanation(result);
    }
}
