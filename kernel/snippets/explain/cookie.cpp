#include "cookie.h"

#include "pagetop.h"
#include "semantic.h"

#include <kernel/info_request/inforequestformatter.h>

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/factors/factors.h>
#include <kernel/snippets/formulae/formula.h>
#include <kernel/snippets/formulae/manual.h>
#include <kernel/snippets/iface/passagereply.h>
#include <kernel/snippets/markers/markers.h>
#include <kernel/snippets/replace/replaceresult.h>
#include <kernel/snippets/sent_match/glue.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/strhl/hilite_mark.h>
#include <kernel/snippets/titles/make_title/util_title.h>
#include <kernel/snippets/util/xml.h>

#include <library/cpp/html/pcdata/pcdata.h>
#include <library/cpp/svnversion/svnversion.h>

#include <util/stream/null.h>
#include <util/stream/printf.h>
#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/charset/wide.h>
#include <util/str_stl.h>
#include <util/generic/hash.h>
#include <util/generic/map.h>
#include <library/cpp/charset/codepage.h>

namespace NSnippets
{
    static const char FilterAuto[]    = "_auto";       // dbgout or all non-meta candidates
    static const char FilterNatural[] = "_natural";    // non-meta
    static const char FilterDebug[]   = "_dbg";        // dbgout
    static const char FilterReplacer[]= "_replacer";   // replacer candidates
    static const char FilterResult[]  = "_result";     // dbgout
    static const char FilterAll[]     = "_all";        // all candidates
    static const char FilterTitle[]   = "_title";      // title candidates
    static const char FilterVersion[]  = "_version";     // dbgout
    static const char FilterSemantic[]= "_semantic";   // i.e. schema.org

    static const size_t MAX_CANDIDATES = 5000;

    const char* CANDIDATE_SOURCE_NAMES[] = {
        "text",
        "link",
        "metadata",
        "faq"
    };

    ECandidateSource UnprefixSourceFilter(TString& filterString)
    {
        size_t colonPos = filterString.find(':');
        ECandidateSource result = CS_TEXT_ARC;
        if (colonPos != TString::npos) {
            unsigned int x = FromStringWithDefault<unsigned int>(TStringBuf(filterString.data(), colonPos), CS_COUNT);
            if (x < CS_COUNT) {
                result = (ECandidateSource)x;
            }
            filterString = filterString.erase(0, colonPos + 1);
        }
        return result;
    }

    TString PrefixSourceFilter(const TString& algoName, ECandidateSource source)
    {
        return ToString<int>((int)source) + ":" + algoName;
    }

    const char* NameForSource(ECandidateSource source)
    {
        int i = (int)source;
        if (i < 0 || i >= (int)Y_ARRAY_SIZE(CANDIDATE_SOURCE_NAMES))
            return "?";
        return CANDIDATE_SOURCE_NAMES[i];
    }

    class TCookieCallback::TImpl
        : public ISnippetCandidateDebugHandler
        , public ISnippetDebugOutputHandler
    {
        static const size_t ITEMS_PER_PAGE = 10;

        enum EDisplayMode {
            DISPLAY_ALL_OR_DBGOUT,
            DISPLAY_NATURAL_CANDS,
            DISPLAY_SPECIFIC_CANDS,
            DISPLAY_ALL,
            DISPLAY_DBGOUT,
            DISPLAY_RESULT,
            DISPLAY_REPLACER,
            DISPLAY_SEMANTIC,
            DISPLAY_TITLE,
            DISPLAY_VERSION,
        };

        struct TAlgoTop;

        struct TCandidate
        {
            const TAlgoTop* Top;
            const TSnip Snip;

            // Filled just before displaying
            size_t NumFactors;
            size_t StartIndex;

            TCandidate(const TAlgoTop* top, const TSnip& snip)
                : Top(top)
                , Snip(snip)
                , NumFactors()
                , StartIndex()
            {
            }
        };

        typedef TSimpleSharedPtr<TCandidate> TCandidateEntry;

        struct TCandidateEntryCmp {
            bool operator()(const TCandidateEntry& a, const TCandidateEntry& b) const {
                return a->Snip.Weight > b->Snip.Weight;
            }
        };

        typedef TPageTop<TCandidateEntry, TCandidateEntryCmp> TCandidateTop;

        typedef THashMap<const char*, int> TFactorNameHash;
        typedef TVector<std::pair<int, double> > TFactorValues; // std::pair<column index, factor value>

        struct TAlgoTop : IAlgoTop
        {
            TCandidateTop* Top;
            TString Algo;
            ECandidateSource Source;
            size_t PushCount;
            size_t TrashCount;
            size_t TitleLikeCount;
            double BestTitleLikeSnip;

            explicit TAlgoTop(TCandidateTop* top, const TString& algo, ECandidateSource source)
                : Top(top)
                , Algo(algo)
                , Source(source)
                , PushCount(0)
                , TrashCount(0)
                , TitleLikeCount(0)
                , BestTitleLikeSnip(INVALID_SNIP_WEIGHT)
            {
            }

            void UpdateFactorStats(const TSnip& snip)
            {
                TConstFactorView a2factors = snip.Factors.CreateConstViewFor(NFactorSlices::EFactorSlice::SNIPPETS_MAIN);
                if (a2factors[A2_MF_READABLE] > 1e-5)
                    ++TrashCount;
                double titleLikePenalty = a2factors[A2_MF_TITLELIKE] * ManualFormula.GetKoef(A2_MF_TITLELIKE);

                if (Source == CS_TEXT_ARC
                    && titleLikePenalty != 0
                    && snip.Weight != INVALID_SNIP_WEIGHT)
                {
                    TitleLikeCount++;

                    if (BestTitleLikeSnip == INVALID_SNIP_WEIGHT || snip.Weight - titleLikePenalty > BestTitleLikeSnip)
                    {
                        BestTitleLikeSnip = snip.Weight - titleLikePenalty;
                    }
                }
            }

            void Push(const TSnip& snip, const TUtf16String& /*title*/ = TUtf16String()) override
            {
                UpdateFactorStats(snip);
                if (Top) {
                    Top->Push(TCandidateEntry(new TCandidate(this, snip)));
                }
                PushCount++;
            }
        };

        struct TDebugOutputEntry {
            const TString Text;
            bool Important;

            TDebugOutputEntry(const TString& text, bool important)
                : Text(text)
                , Important(important)
            {
            }
        };

        struct TReplacerCandidate {
            const TString Name;
            const TReplaceResult ReplaceResult;
            const TString Comment;

            TReplacerCandidate(const TString& name, const TReplaceResult& replaceResult, const TString& comment)
                : Name(name)
                , ReplaceResult(replaceResult)
                , Comment(comment)
            {
            }
        };

        struct TSnipTitleCandidate {
            TSnipTitle SnipTitle;
            ETitleCandidateSource TitleSource;
            TSnipTitleCandidate(const TSnipTitle& snipTitle, ETitleCandidateSource titleSource)
                : SnipTitle(snipTitle)
                , TitleSource(titleSource)
            {}
        };

        typedef TSimpleSharedPtr<TSnipTitleCandidate> TTitleCandidateEntry;

        struct TSnipTitleCmp {
            bool operator()(const TTitleCandidateEntry& a, const TTitleCandidateEntry& b) const {
                return a->SnipTitle.GetPLMScore() > b->SnipTitle.GetPLMScore();
            }
        };

        typedef TList<TAlgoTop> TTops;
        typedef TVector<TDebugOutputEntry> TDebugOutputEntries;
        typedef TVector<TReplacerCandidate> TReplacerCandidates;
        typedef TPageTop<TTitleCandidateEntry, TSnipTitleCmp> TTitleTop;

        TString AlgoNameFilter;
        ECandidateSource SourceFilter;
        EDisplayMode DisplayMode;
        ui32 PageNumber;
        size_t StartOffset;
        size_t ItemLimit;
        size_t Count;

        TDebugOutputEntries DebugOutput;
        TReplacerCandidates ReplacerCandidates;
        TTops Tops;
        TCandidateTop Candidates;
        TTitleTop TitleTop;
        bool IsJson;
        TPassageReply Reply;
        TDocInfos DocInfos;
        TMarkersMask SnipMarkers;

        void FillCandidates(TVector<TCandidateEntry>& page, TFactorNameHash& factorNames, TFactorValues& factorValues) const;
        void FillAlgoNames(PtrInfoDataTable& algoName, ECandidateSource src) const;
        void ProduceFilterNamePicker(TInfoRequestFormatter& formatter, bool hasImportantDbg) const;
        void ProduceSnippetCandidates(IInfoDataTable* table, ui32& pageCount) const;
        void ProduceDebugOutput(IInfoDataTable* result) const;
        void ProduceReplacerDebug(IInfoDataTable* result) const;
        void ProduceSemanticMarkup(IInfoDataTable* result) const;
        void ProduceResult(IInfoDataTable* result) const;
        void ProduceVersion(IInfoDataTable* result) const;
        void ProduceTitleCandidates(IInfoDataTable* result, ui32& pageCount) const;
        TString FormatSnippet(const TCandidate& snippet) const;
        TString GetReplacerInfo(const TReplacerCandidate& replaceCandidate) const;

    public:
        TImpl(const TSnipInfoReqParams& params)
            : AlgoNameFilter(params.AlgoName)
            , PageNumber(params.PageNumber)
            , StartOffset((PageNumber >= 1 ? PageNumber - 1 : 0) * ITEMS_PER_PAGE)
            , ItemLimit(Min(StartOffset + ITEMS_PER_PAGE, MAX_CANDIDATES))
            , Count(Max(ItemLimit, StartOffset) - StartOffset)
            , Candidates(Count, StartOffset)
            , TitleTop(Count, StartOffset)
            , IsJson(params.TableType == INFO_JSON)
        {
            SourceFilter = UnprefixSourceFilter(AlgoNameFilter);

            if (AlgoNameFilter == FilterAuto || AlgoNameFilter.empty()) {
                DisplayMode = DISPLAY_ALL_OR_DBGOUT;
            }
            else if (AlgoNameFilter == FilterDebug) {
                DisplayMode = DISPLAY_DBGOUT;
            }
            else if (AlgoNameFilter == FilterReplacer) {
                DisplayMode = DISPLAY_REPLACER;
            }
            else if (AlgoNameFilter == FilterSemantic) {
                DisplayMode = DISPLAY_SEMANTIC;
            }
            else if (AlgoNameFilter == FilterResult) {
                DisplayMode = DISPLAY_RESULT;
            }
            else if (AlgoNameFilter == FilterVersion) {
                DisplayMode = DISPLAY_VERSION;
            }
            else if (AlgoNameFilter == FilterNatural) {
                DisplayMode = DISPLAY_NATURAL_CANDS;
            }
            else if (AlgoNameFilter == FilterAll) {
                DisplayMode = DISPLAY_ALL;
            }
            else if (AlgoNameFilter == FilterTitle) {
                DisplayMode = DISPLAY_TITLE;
            }
            else {
                DisplayMode = DISPLAY_SPECIFIC_CANDS;
            }
        }

        IAlgoTop* AddTop(const char* algo, ECandidateSource source) override
        {
            bool wantTop =
                    (DisplayMode == DISPLAY_SPECIFIC_CANDS && AlgoNameFilter == algo && SourceFilter == source)
                    || (DisplayMode == DISPLAY_NATURAL_CANDS && (source == CS_TEXT_ARC || source == CS_LINK_ARC))
                    || (DisplayMode == DISPLAY_ALL_OR_DBGOUT)
                    || (DisplayMode == DISPLAY_ALL);

            IAlgoTop* top = FindTop(algo, source);
            if (top == nullptr) {
                Tops.push_front(TAlgoTop(wantTop ? &Candidates : nullptr, algo, source));
                top = &Tops.front();
            }
            return top;
        }

        void AddTitleCandidate(const TSnipTitle& title, ETitleCandidateSource source) override {
            TitleTop.Push(TTitleCandidateEntry(new TSnipTitleCandidate(title, source)));
        }

        IAlgoTop* FindTop(const char* algo, ECandidateSource source)
        {
            for (TTops::iterator ii = Tops.begin(); ii != Tops.end(); ++ii) {
                if (ii->Algo == algo && ii->Source == source) {
                    return &(*ii);
                }
            }
            return nullptr;
        }

        double GetShareOfTrashCandidates(bool isByLink) const;

        void GetExplanation(IOutputStream& result) const;

        void Print(bool important, const char* text, ...) override
        {
            TString formatted;
            va_list params;
            va_start(params, text);
            TStringOutput so(formatted);
            Printf(so, text, params);
            va_end(params);

            DebugOutput.push_back(TDebugOutputEntry(formatted, important));
        };

        void ReplacerPrint(const TString& replacerName, const TReplaceResult& replaceResult, const TString& comment) override
        {
            ReplacerCandidates.push_back(TReplacerCandidate(replacerName, replaceResult, comment));
        }

        void OnPassageReply(const TPassageReply& reply)
        {
            Reply = reply;
        }

        void OnDocInfos(const TDocInfos& docInfos)
        {
            DocInfos = docInfos;
        }

        void OnMarkers(const TMarkersMask& markers) {
            SnipMarkers = markers;
        }

        void OnBestFinal(const TSnip& snip, bool isByLink)
        {
            if (isByLink)
                return;

            size_t totalTitleLike = 0;
            size_t totalTextCands = 0;
            bool titleLikeChangesResult = false;
            for (TTops::const_iterator top = Tops.begin(), end = Tops.end(); top != end; ++top) {
                if (top->Source != CS_TEXT_ARC) {
                    continue;
                }
                totalTextCands += top->PushCount;
                totalTitleLike += top->TitleLikeCount;

                if (top->BestTitleLikeSnip != INVALID_SNIP_WEIGHT
                    && snip.Weight != INVALID_SNIP_WEIGHT
                    && top->BestTitleLikeSnip > snip.Weight + 1e-10)
                {
                    titleLikeChangesResult = true;
                }
            }

            if (titleLikeChangesResult) {
                Print(true, "Natural snippet changed because of title-like fragment pessimization");
            }
            if (totalTextCands != 0) {
                Print(false, "Pessimized title-like candidate count: %.3f%%", ((double)totalTitleLike) * 100.0 / totalTextCands);
            }
        }
    };

    static TString PrintFloat(double f)
    {
        TString result = Sprintf("%.8f", f);

        int i;
        for (i = ((int)result.size()) - 1; i > 0; --i) {
            if (result[i] == '0') {
                continue;
            }
            if (result[i] == '.') {
                --i;
            }
            break;
        }

        result.resize(i+1, '0');
        return result;
    }

    TString TCookieCallback::TImpl::FormatSnippet(const TCandidate& cand) const
    {
        const TUtf16String o(u"<b>");
        const TUtf16String c(u"</b>");
        const THiliteMark oc(o, c);
        TString snippet;
        TVector<TZonedString> tmp = cand.Snip.GlueToZonedVec(true);
        for (size_t i = 0; i != tmp.size(); ++i) {
            if (cand.Top->Source == CS_LINK_ARC) {
                tmp[i] = TGluer::CutTrash(tmp[i]);
            }
            tmp[i].Zones[+TZonedString::ZONE_MATCH].Mark = &oc;
            const TString raw = TGluer::GlueToHtmlEscapedUTF8String(tmp[i]);
            const char* strStart = raw.data();
            const char* strEnd = raw.data() + raw.length();
            if (raw.StartsWith("...")) {
                strStart += 3;
            }
            if (raw.EndsWith("...")) {
                strEnd -= 3;
            }
            if (strEnd > strStart) {
                snippet.append(strStart, strEnd);
            }
            if (i + 1 != tmp.size()) {
                snippet.append("&nbsp;<span class=\"b-serp-item__text_passage\">&hellip;</span> ");
            }
        }

        return EncodeTextForXml10(TString::Join("<div class=\"kuka-dynamic-snip-width\">", snippet, "</div>"), !IsJson);
    }

    double TCookieCallback::TImpl::GetShareOfTrashCandidates(bool isByLink) const
    {
        int totalCandidates = 0;
        int trashCandidates = 0;
        ECandidateSource neededSource = isByLink ? CS_LINK_ARC : CS_TEXT_ARC;
        for (const TAlgoTop& top : Tops) {
            if (top.Source == neededSource) {
                totalCandidates += top.PushCount;
                trashCandidates += top.TrashCount;
            }
        }
        return totalCandidates > 0 ? static_cast<double>(trashCandidates) / totalCandidates : 0.;
    }

    void TCookieCallback::TImpl::FillCandidates(TVector<TCandidateEntry>& page, TFactorNameHash& factorNames, TFactorValues& factorValues) const
    {
        for (TVector<TCandidateEntry>::const_iterator jj = page.begin(), jjend = page.end(); jj != jjend; ++jj) {
            TCandidate& cand = **jj;
            cand.NumFactors = cand.Snip.Factors.Size();
            cand.StartIndex = factorValues.size();
            for (size_t kk = 0; kk < cand.Snip.Factors.Size(); ++kk) {
                const char* factorName = cand.Snip.Factors.GetDomain().GetFactorInfo(kk).GetFactorName();
                std::pair<TFactorNameHash::iterator, bool> index = factorNames.insert(std::make_pair(factorName, 0));
                if (index.second) {
                    index.first->second = factorNames.size() - 1;
                }
                factorValues.push_back(std::make_pair(index.first->second, cand.Snip.Factors[kk]));
            }
        }
    }

    void TCookieCallback::TImpl::ProduceSnippetCandidates(IInfoDataTable* table, ui32& pageCount) const
    {
        TFactorNameHash factorNames; // factor name -> column index
        TFactorValues factorValues; // concatenated values for all candidates

        TVector<TCandidateEntry> sortedCands(Candidates.ToSortedVector());
        FillCandidates(sortedCands, factorNames, factorValues);

        TVector<TInfoDataCell> cells;
        size_t reservedCols = 3;
        cells.resize(reservedCols + factorNames.size());
        cells[0] = TInfoDataCell("Snippet");
        cells[1] = TInfoDataCell("Weight");
        cells[2] = TInfoDataCell("Algo");
        for (TFactorNameHash::iterator ii = factorNames.begin(); ii != factorNames.end(); ++ii) {
            cells[reservedCols + ii->second] = TInfoDataCell(TString(ii->first));
        }
        table->AddRow(cells, RT_HEADER);

        for (size_t i = 0; i < sortedCands.size(); ++i) {
            TCandidate& cand = *sortedCands[i];
            cells.clear();
            cells.push_back(TInfoDataCell(FormatSnippet(cand), false));
            cells.push_back(TInfoDataCell(PrintFloat(cand.Snip.Weight)));
            TString algoName = Sprintf("%s: %s", NameForSource(cand.Top->Source), (cand.Top->Algo).data());
            cells.push_back(TInfoDataCell(algoName));
            size_t jend = cand.StartIndex + cand.NumFactors;
            for (size_t j = cand.StartIndex; j != jend; ++j) {
                TFactorValues::value_type& value = factorValues[j];
                cells.push_back(TInfoDataCell(PrintFloat(value.second)));
            }
            table->AddRow(cells, RT_DATA);
        }

        size_t totalCandidates = 0;
        for (TTops::const_iterator ii = Tops.begin(), end = Tops.end(); ii != end; ++ii) {
            if (ii->Top != nullptr)
                totalCandidates += ii->PushCount;
        }

        if (totalCandidates)
            pageCount = (totalCandidates + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE;
    }

    void TCookieCallback::TImpl::ProduceDebugOutput(IInfoDataTable* result) const
    {
        TVector<TInfoDataCell> cells;

        cells.push_back(TInfoDataCell("Message"));
        result->AddRow(cells, RT_HEADER);

        for (TDebugOutputEntries::const_iterator ii = DebugOutput.begin(); ii != DebugOutput.end(); ++ii) {
            cells.clear();
            TString val = EncodeTextForXml10(ii->Text, false);
            if (ii->Important) {
                val = TString::Join("<b>", val, "</b>");
            }

            cells.push_back(TInfoDataCell(val, true));
            result->AddRow(cells, RT_DATA);
        }
    }

    TString TCookieCallback::TImpl::GetReplacerInfo(const TReplacerCandidate& replacerCandidate) const
    {
        TString result = TString::Join("<b>", EncodeHtmlPcdata(replacerCandidate.Name, true), "</b>");
        const TReplaceResult& replaceResult = replacerCandidate.ReplaceResult;

        if (replaceResult.GetTitle()) {
            const TUtf16String& title = replaceResult.GetTitle()->GetTitleString();
            result.append(TString::Join("<br><br>Title:<br>", WideToUTF8(title)));
        }

        if (replaceResult.GetText().size()) {
            result.append(TString::Join("<br><br>Headline:<br>", WideToUTF8(replaceResult.GetText())));
        }

        if (replacerCandidate.Comment.size()) {
            result.append(TString::Join("<br><br>Comment:<br>", replacerCandidate.Comment));
        }
        return result;
    }

    void TCookieCallback::TImpl::ProduceReplacerDebug(IInfoDataTable* result) const
    {
        TVector<TInfoDataCell> cells;

        cells.push_back(TInfoDataCell("Replacer"));
        result->AddRow(cells, RT_HEADER);

        for (TReplacerCandidates::const_iterator ii = ReplacerCandidates.begin(); ii != ReplacerCandidates.end(); ++ii) {
            cells.clear();
            cells.push_back(TInfoDataCell(GetReplacerInfo(*ii)));
            result->AddRow(cells, RT_DATA);
        }
    }

    static inline TUtf16String Rehighlight(const TUtf16String& s)
    {
        TUtf16String res;
        for (size_t i = 0; i < s.size(); ) {
            if (s[i] == 0x07 && i + 1 < s.size()) {
                if (s[i + 1] == '[' || s[i + 1] == ']') {
                    res += UTF8ToWide(s[i + 1] == '[' ? "<b>" : "</b>");
                    i += 2;
                    continue;
                }
            }
            res += s[i];
            ++i;
        }
        return res;
    }

    void TCookieCallback::TImpl::ProduceResult(IInfoDataTable* result) const
    {
        TVector<TInfoDataCell> cells;

        cells.push_back(TInfoDataCell(""));
        cells.push_back(TInfoDataCell("Results"));
        result->AddRow(cells, RT_HEADER);

        cells.clear();
        cells.push_back(TInfoDataCell("Title: "));
        cells.push_back(TInfoDataCell(EncodeTextForXml10(WideToUTF8(Rehighlight(Reply.GetTitle())), false)));
        result->AddRow(cells, RT_DATA);

        if (!Reply.GetHeadline().empty()) {
            cells.clear();
            cells.push_back(TInfoDataCell("Headline: "));
            cells.push_back(TInfoDataCell(EncodeTextForXml10(WideToUTF8(Rehighlight(Reply.GetHeadline())), false)));
            result->AddRow(cells, RT_DATA);
        }

        for (size_t i = 0; i < Reply.GetPassages().size(); ++i) {
            cells.clear();
            cells.push_back(TInfoDataCell(Reply.GetPassagesType() == 0 ? "Passage: " : "ByLink: "));
            cells.push_back(TInfoDataCell(EncodeTextForXml10(WideToUTF8(Rehighlight(Reply.GetPassages()[i])), false)));
            result->AddRow(cells, RT_DATA);
        }

        if (!Reply.GetHeadlineSrc().empty()) {
            cells.clear();
            result->AddRow(cells, RT_DATA);
            cells.push_back(TInfoDataCell("HeadlineSrc: "));
            cells.push_back(TInfoDataCell(EncodeTextForXml10(Reply.GetHeadlineSrc(), false)));
            result->AddRow(cells, RT_DATA);
        }

        if (!Reply.GetSpecSnippetAttrs().empty()) {
            cells.clear();
            result->AddRow(cells, RT_DATA);
            cells.push_back(TInfoDataCell("SpecSnipAttrs: "));
            cells.push_back(TInfoDataCell(EncodeTextForXml10(Reply.GetSpecSnippetAttrs(), false)));
            result->AddRow(cells, RT_DATA);
        }

        bool haveAttrs = false;
        for (size_t i = 0; i < Reply.GetPassagesAttrs().size(); ++i) {
            if (Reply.GetPassagesAttrs()[i].size()) {
                haveAttrs = true;
                break;
            }
        }
        if (haveAttrs) {
            for (size_t i = 0; i < Reply.GetPassagesAttrs().size(); ++i) {
                cells.clear();
                cells.push_back(TInfoDataCell("PassageAttr: "));
                cells.push_back(TInfoDataCell(EncodeTextForXml10(Reply.GetPassagesAttrs()[i]), false));
                result->AddRow(cells, RT_DATA);
            }
        }

        if (!Reply.GetLinkAttrs().empty()) {
            cells.clear();
            result->AddRow(cells, RT_DATA);
            cells.push_back(TInfoDataCell("LinkAttrs: "));
            cells.push_back(TInfoDataCell(EncodeTextForXml10(Reply.GetLinkAttrs(), false)));
            result->AddRow(cells, RT_DATA);
        }

        if (!Reply.GetSchemaVthumb().empty()) {
            cells.clear();
            result->AddRow(cells, RT_DATA);
            cells.push_back(TInfoDataCell("SchemaVthumb: "));
            cells.push_back(TInfoDataCell(EncodeTextForXml10(Reply.GetSchemaVthumb(), false)));
            result->AddRow(cells, RT_DATA);
        }

        if (!Reply.GetClickLikeSnip().empty()) {
            cells.clear();
            result->AddRow(cells, RT_DATA);
            cells.push_back(TInfoDataCell("ClickLikeSnip: "));
            cells.push_back(TInfoDataCell(EncodeTextForXml10(Reply.GetClickLikeSnip(), false)));
            result->AddRow(cells, RT_DATA);
        }

        if (!Reply.GetHilitedUrl().empty()) {
            cells.clear();
            result->AddRow(cells, RT_DATA);
            cells.push_back(TInfoDataCell("HilitedUrl: "));
            cells.push_back(TInfoDataCell(EncodeTextForXml10(WideToUTF8(Rehighlight(UTF8ToWide(Reply.GetHilitedUrl()))), false)));
            result->AddRow(cells, RT_DATA);
        }

        if (!Reply.GetUrlMenu().empty()) {
            cells.clear();
            result->AddRow(cells, RT_DATA);
            cells.push_back(TInfoDataCell("UrlMenu: "));
            cells.push_back(TInfoDataCell(EncodeTextForXml10(Reply.GetUrlMenu(), false)));
            result->AddRow(cells, RT_DATA);
        }

        TString enabledMarkers = PrintEnabled(SnipMarkers);
        if (enabledMarkers) {
            cells.clear();
            cells.push_back(TInfoDataCell("Markers: "));
            cells.push_back(TInfoDataCell(enabledMarkers));
            result->AddRow(cells, RT_DATA);
        }
    }

    static TString LnToBr(const TString& s) {
        TString res;
        for (size_t i = 0; i < s.size(); ++i) {
            if (s[i] == '\n') {
                res += "<br>\n";
            } else {
                res += s[i];
            }
        }
        return res;
    }

    void TCookieCallback::TImpl::ProduceVersion(IInfoDataTable* result) const
    {
        TVector<TInfoDataCell> cells;

        cells.push_back(TInfoDataCell("Version"));
        result->AddRow(cells, RT_HEADER);

        cells.clear();
        cells.push_back(TInfoDataCell(EncodeTextForXml10(LnToBr(GetProgramSvnVersion()), false)));
        result->AddRow(cells, RT_DATA);
    }

    void TCookieCallback::TImpl::ProduceSemanticMarkup(IInfoDataTable* result) const
    {
        PrintSemanticMarkup(DocInfos, result);
    }

    void TCookieCallback::TImpl::ProduceTitleCandidates(IInfoDataTable* result, ui32& pageCount) const
    {
        TVector<TInfoDataCell> cells;
        cells.push_back(TInfoDataCell("Title"));
        cells.push_back(TInfoDataCell("Source"));
        cells.push_back(TInfoDataCell("PLM score"));
        cells.push_back(TInfoDataCell("Idf sum"));
        cells.push_back(TInfoDataCell("User idf sum"));
        cells.push_back(TInfoDataCell("Synonyms count"));
        cells.push_back(TInfoDataCell("Query words ratio"));
        result->AddRow(cells, RT_HEADER);

        TVector<TTitleCandidateEntry> sortedCandidates = TitleTop.ToSortedVector();
        for (size_t i = 0; i < sortedCandidates.size(); ++i) {
            TSnipTitleCandidate& candidate = *sortedCandidates[i];
            cells.clear();
            cells.push_back(TInfoDataCell(WideToUTF8(candidate.SnipTitle.GetTitleString())));
            switch (candidate.TitleSource) {
                case TS_NATURAL:
                    cells.push_back(TInfoDataCell("natural"));
                    break;
                case TS_HEADER_BASED:
                    cells.push_back(TInfoDataCell("header based"));
                    break;
                case TS_NATURAL_WITH_HEADER_BASED:
                    cells.push_back(TInfoDataCell("natural with header based"));
                    break;
                default:
                    cells.push_back(TInfoDataCell("unknown"));
            }
            TString plmString = PrintFloat(candidate.SnipTitle.GetPLMScore());
            if (!candidate.SnipTitle.GetTitleString().size())
                plmString = "-infinity";
            cells.push_back(TInfoDataCell(plmString));
            cells.push_back(TInfoDataCell(PrintFloat(candidate.SnipTitle.GetLogMatchIdfSum())));
            cells.push_back(TInfoDataCell(PrintFloat(candidate.SnipTitle.GetMatchUserIdfSum())));
            cells.push_back(TInfoDataCell(Sprintf("%d", candidate.SnipTitle.GetSynonymsCount())));
            cells.push_back(TInfoDataCell(PrintFloat(candidate.SnipTitle.GetQueryWordsRatio())));
            result->AddRow(cells, RT_DATA);
        }
        pageCount = (TitleTop.GetPushCount() + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE;
    }

    void TCookieCallback::TImpl::FillAlgoNames(PtrInfoDataTable& algoNames, ECandidateSource src) const
    {
        TVector<std::pair<TString, size_t> > algos;
        int naturalCandCount = 0;
        TString srcName(NameForSource(src));

        for (TTops::const_iterator ii = Tops.begin(), iiend = Tops.end(); ii != iiend; ++ii) {
            const TAlgoTop& top = *ii;
            if (top.Source == src && top.PushCount) {
                algos.push_back(std::make_pair(top.Algo, top.PushCount));
                naturalCandCount += top.PushCount;
            }
        }

        if (!naturalCandCount)
            return;

        SortBy(algos, [](const std::pair<TString, size_t>& x) { return x.first; });

        if (src == CS_TEXT_ARC || src == CS_LINK_ARC)
            algoNames->AddRow(
                Sprintf("%s: all candidates (%d)", srcName.data(), naturalCandCount),
                PrefixSourceFilter(FilterNatural, src));
        for (int j = 0; j < algos.ysize(); ++j) {
            algoNames->AddRow(
                Sprintf("%s: %s (%" PRISZT ")", srcName.data(), (algos[j].first).data(), algos[j].second),
                PrefixSourceFilter(algos[j].first, src));
        }
    }

    void TCookieCallback::TImpl::ProduceFilterNamePicker(TInfoRequestFormatter& formatter, bool hasImportantDbg) const
    {
        PtrInfoDataTable algoNames = formatter.AddDataTable("SnippetsSectionNames");

        const char* autoViewMode = hasImportantDbg ? "important debug entries" : "all candidates";

        algoNames->AddRow(TStringBuilder() << "Auto (" << autoViewMode << ")", FilterAuto);

        for (int i = 0; i < CS_COUNT; ++i)
            FillAlgoNames(algoNames, (ECandidateSource)i);
        algoNames->AddRow("All sources: all candidates", FilterAll);
        if (TitleTop.GetPushCount())
            algoNames->AddRow(TStringBuilder() << "Title candidates (" << (int)TitleTop.GetPushCount() << ")", FilterTitle);
        if (DebugOutput.size())
            algoNames->AddRow(TStringBuilder() << "Debug output (" << DebugOutput.size() << ")", FilterDebug);
        if (ReplacerCandidates.size())
            algoNames->AddRow(TStringBuilder() << "Replacers (" << ReplacerCandidates.size() << ")", FilterReplacer);
        if (HasSemanticMarkup(DocInfos))
            algoNames->AddRow("Semantic markup data", FilterSemantic);
        algoNames->AddRow("Result", FilterResult);
        algoNames->AddRow("Version", FilterVersion);
    }

    void TCookieCallback::TImpl::GetExplanation(IOutputStream& result) const
    {
        TBufferOutput outBuf;
        TInfoRequestFormatter formatter(IsJson, outBuf);
        PtrInfoDataTable table = formatter.AddDataTable("Snippets");

        ui32 pageCount = 1;

        bool hasImportantDbg = false;
        for (size_t i = 0; i < DebugOutput.size(); ++i) {
            if (DebugOutput[i].Important) {
                hasImportantDbg = true;
                break;
            }
        }

        bool showImportantMsgs = (DisplayMode == DISPLAY_ALL_OR_DBGOUT && hasImportantDbg);
        if (DisplayMode == DISPLAY_DBGOUT || showImportantMsgs) {
            ProduceDebugOutput(table.Get());
        }
        else if (DisplayMode == DISPLAY_REPLACER) {
            ProduceReplacerDebug(table.Get());
        }
        else if (DisplayMode == DISPLAY_SEMANTIC) {
            ProduceSemanticMarkup(table.Get());
        }
        else if (DisplayMode == DISPLAY_RESULT) {
            ProduceResult(table.Get());
        } else if (DisplayMode == DISPLAY_VERSION) {
            ProduceVersion(table.Get());
        } else if (DisplayMode == DISPLAY_TITLE) {
            ProduceTitleCandidates(table.Get(), pageCount);
        }
        else {
            ProduceSnippetCandidates(table.Get(), pageCount);
        }

        ProduceFilterNamePicker(formatter, hasImportantDbg);
        formatter.SetPageCount(pageCount);
        formatter.SetPageNumber(PageNumber);

        formatter.Finalize();
        result.Write(outBuf.Buffer().Data(), outBuf.Buffer().Size());
    }

    TCookieCallback::TCookieCallback(const TSnipInfoReqParams& params)
        : Impl(new TImpl(params))
    {

    }

    ISnippetCandidateDebugHandler* TCookieCallback::GetCandidateHandler()
    {
        return Impl.Get();
    }

    ISnippetDebugOutputHandler* TCookieCallback::GetDebugOutput()
    {
        return Impl.Get();
    }

    void TCookieCallback::OnPassageReply(const TPassageReply& reply, const TEnhanceSnippetConfig&)
    {
        Impl->OnPassageReply(reply);
    }

    void TCookieCallback::OnDocInfos(const TDocInfos& docInfos)
    {
        Impl->OnDocInfos(docInfos);
    }
    void TCookieCallback::OnMarkers(const TMarkersMask& markers) {
        Impl->OnMarkers(markers);
    }

    double TCookieCallback::GetShareOfTrashCandidates(bool isByLink) const
    {
        return Impl->GetShareOfTrashCandidates(isByLink);
    }

    void TCookieCallback::GetExplanation(IOutputStream& result) const
    {
        Impl->GetExplanation(result);
    }

    void TCookieCallback::OnBestFinal(const TSnip& snip, bool isByLink)
    {
        Impl->OnBestFinal(snip, isByLink);
    }
}
