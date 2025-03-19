#include "snip_proto_2.h"

#include <kernel/snippets/algo/one_span.h>
#include <kernel/snippets/algo/video_span.h>
#include <kernel/snippets/algo/two_span.h>
#include <kernel/snippets/algo/redump.h>
#include <kernel/snippets/calc_dssm/calc_dssm.h>
#include <kernel/snippets/config/config.h>
#include <kernel/snippets/cut/cut.h>
#include <kernel/snippets/explain/top_candidates.h>
#include <kernel/snippets/factors/factors.h>
#include <kernel/snippets/sent_info/beautify.h>
#include <kernel/snippets/sent_info/sent_info.h>
#include <kernel/snippets/sent_match/len_chooser.h>
#include <kernel/snippets/sent_match/length.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/sent_match/restr.h>
#include <kernel/snippets/sent_match/sent_match.h>
#include <kernel/snippets/titles/make_title/make_title.h>
#include <kernel/snippets/titles/make_title/util_title.h>
#include <kernel/snippets/uni_span_iter/uni_span_iter.h>
#include <kernel/snippets/weight/weighter.h>
#include <kernel/snippets/wordstat/wordstat_data.h>

#include <util/generic/bitmap.h>
#include <util/generic/hash.h>
#include <util/generic/list.h>
#include <util/generic/vector.h>
#include <util/string/util.h>


namespace NSnippets {

    class TOnSnip
    {
    private:
        TList<TSnip> SnipList;
        TSnip BestSnip;
        TSnip BestOneFragmentSnip;
        const TSnipTitle& Title;
    private:
        TDynBitMap GetBitSetFromSnip(const TSnip &snip) const;
    public:
        TOnSnip(const TSnipTitle& title);

        void operator()(const TSnip& s);
        void OnOneFragmentSymbolCut(const TSnip& s);
        TVector<TDynBitMap> GetBitMasks() const;

        const TSnip& GetBestSnip() const {
            return BestSnip;
        }

        const TSnip& GetBestOneFragmentSnip() const {
            return BestOneFragmentSnip;
        }
    };

    TOnSnip::TOnSnip(const TSnipTitle& title)
            : Title(title)
    {
    }

    void TOnSnip::operator()(const TSnip& s)
    {
        if (s.Snips.empty()) {
            return;
        }

        SnipList.push_back(s);

        if (s.Weight > BestSnip.Weight) {
            BestSnip = s;
        }
    }

    void TOnSnip::OnOneFragmentSymbolCut(const TSnip& s) {
        if (s.Snips.size() == 1 && s.Weight > BestOneFragmentSnip.Weight) {
            BestOneFragmentSnip = s;
        }
    }

    TDynBitMap TOnSnip::GetBitSetFromSnip(const TSnip &snip) const {
        if (snip.Snips.empty()) {
            return TDynBitMap();
        }

        const TSentsMatchInfo& sMInfo = *snip.Snips.begin()->GetSentsMatchInfo();
        const TSentsInfo& sInfo = sMInfo.SentsInfo;
        TWordStatData wordStat(sMInfo.Query, sInfo.MaxN);
        for (TSnip::TSnips::const_iterator it = snip.Snips.begin(); it != snip.Snips.end(); ++it) {
            wordStat.PutSpan(sMInfo, it->GetFirstWord(), it->GetLastWord());
        }
        return NSnipWordSpans::GenUsefullWordsBSet(wordStat.SeenLikePos, sMInfo.Query);

    }

    TVector<TDynBitMap> TOnSnip::GetBitMasks() const {
        // collect bitset for every already build candidates with title
        TVector<TDynBitMap> bitMasks;
        bitMasks.reserve(SnipList.size());
        const TDynBitMap titleMask = GetBitSetFromSnip(Title.GetTitleSnip());
        for (TList<TSnip>::const_iterator it = SnipList.begin(); it != SnipList.end(); ++it) {
            bitMasks.push_back(GetBitSetFromSnip(*it) | titleMask);
        }
        return bitMasks;
    }

    class TGetBestSnip
    {
    private:
        const TSentsMatchInfo& SentsMatchInfo;
        const TSnipTitle& Title;
        const TConfig& Cfg;
        const TString& Url;
        const TLengthChooser& LenCfg;
        const TWordSpanLen& WordSpanLen;
        ISnippetsCallback& Callback;
        TTopCandidateCallback* FactSnipCandidatesCallback;

        bool IsByLink;
        float MaxLenMultiplier;
        bool DontGrow;
        const TSchemaOrgArchiveViewer* SchemaOrgViewer;

        ISnippetCandidateDebugHandler* TopCallback;
        ISnippetCandidateDebugHandler* TopFactSnipCandidatesCallback;

        THolder<TSkippedRestr> SkipRestr;
        THolder<TParaRestr> ParaRestr;
        THolder<TQualRestr> QualRestr;
        THolder<TSegmentRestr> SegRestr;
        THolder<TSimilarRestr> SimRestr;
        TRestr Restr;
        TRestr GrowRestr;
    private:
        void FillRestrictions();
        void FillByLinkRestr();
        void FillRestr();
        void DoRedump();
        void DoAlgos(TOnSnip& onSnip);
        float GetMaxSnipLen(int parts = 0);
        float GetMaxPartLen();
        float GetMaxSnipCount();
        float GetMaxLenForLPFactors();
        // Issue: SNIPPETS-1154
        bool WantNFrag(int n);
        bool CutNFrags();

    public:
        TGetBestSnip(
                const TSentsMatchInfo& sentsMatchInfo
                , const TSnipTitle& unnaturalTitle
                , const TConfig& cfg
                , const TString& url
                , const TLengthChooser& lenCfg
                , const TWordSpanLen& wordSpanLen
                , ISnippetsCallback& callback
                , TTopCandidateCallback* fsCallback
                , bool isByLink
                , float maxLenMultiplier
                , bool dontGrow
                , const TSchemaOrgArchiveViewer* schemaOrgViewer
        );

        TSnip GetSnip(TSnip* oneFragmentSnip);
    };

    TSnip GetBestSnip(
            const TSentsMatchInfo& sentsMatchInfo
            , const TSnipTitle& unnaturalTitle
            , const TConfig& cfg
            , const TString& url
            , const TLengthChooser& lenCfg
            , const TWordSpanLen& wordSpanLen
            , ISnippetsCallback& callback
            , TTopCandidateCallback* fsCallback
            , bool isByLink
            , float maxLenMultiplier
            , bool dontGrow
            , TSnip* oneFragmentSnip
            , const TSchemaOrgArchiveViewer* schemaOrgViewer
    )
    {
        TGetBestSnip impl(sentsMatchInfo, unnaturalTitle, cfg, url, lenCfg, wordSpanLen, callback, fsCallback, isByLink, maxLenMultiplier, dontGrow, schemaOrgViewer);
        return impl.GetSnip(oneFragmentSnip);
    }

    TSnip GetBestSnip(
            const TSentsMatchInfo& sentsMatchInfo
            , const TSnipTitle& unnaturalTitle
            , const TConfig& cfg
            , const TString& url
            , const TLengthChooser& lenCfg
            , const TWordSpanLen& wordSpanLen
            , ISnippetsCallback& callback
            , bool isByLink
            , float maxLenMultiplier
            , bool dontGrow
            , TSnip* oneFragmentSnip
            , const TSchemaOrgArchiveViewer* schemaOrgViewer
    )
    {
        return GetBestSnip(sentsMatchInfo, unnaturalTitle, cfg, url, lenCfg, wordSpanLen,callback, nullptr, isByLink, maxLenMultiplier, dontGrow, oneFragmentSnip, schemaOrgViewer);
    }

    void TGetBestSnip::FillRestrictions()
    {
        if (IsByLink)
            FillByLinkRestr();
        else
            FillRestr();
    }

    void TGetBestSnip::FillByLinkRestr()
    {
        SkipRestr.Reset(new TSkippedRestr(true, SentsMatchInfo.SentsInfo, Cfg));
        SimRestr.Reset(new TSimilarRestr(SentsMatchInfo));
        Restr.AddRestr(SkipRestr.Get());
        Restr.AddRestr(SimRestr.Get());
    }

    void TGetBestSnip::FillRestr()
    {
        SkipRestr.Reset(new TSkippedRestr(false, SentsMatchInfo.SentsInfo, Cfg));
        ParaRestr.Reset(new TParaRestr(SentsMatchInfo.SentsInfo));
        QualRestr.Reset(new TQualRestr(SentsMatchInfo));
        SegRestr.Reset(new TSegmentRestr(SentsMatchInfo));
        SimRestr.Reset(new TSimilarRestr(SentsMatchInfo));

        ISnippetTextDebugHandler* textCallback = Callback.GetTextHandler(IsByLink);
        if (textCallback) {
            textCallback->AddRestrictions("skip", *SkipRestr.Get());
            if (!Cfg.DebugOnlySkipRestr()) {
                textCallback->AddRestrictions("para", *ParaRestr.Get());
                textCallback->AddRestrictions("qual", *QualRestr.Get());
                textCallback->AddRestrictions("seg", *SegRestr.Get());
                textCallback->AddRestrictions("sim", *SimRestr.Get());
            }
        }

        Restr.AddRestr(SkipRestr.Get());
        Restr.AddRestr(ParaRestr.Get());
        Restr.AddRestr(QualRestr.Get());
        Restr.AddRestr(SegRestr.Get());
        Restr.AddRestr(SimRestr.Get());

        GrowRestr.AddRestr(SkipRestr.Get());
    }

    float TGetBestSnip::GetMaxSnipCount()
    {
        if (IsByLink) {
            return Cfg.GetMaxByLinkSnipCount();
        } else {
            return Cfg.GetMaxSnipCount();
        }
    }

    float TGetBestSnip::GetMaxSnipLen(int parts)
    {
        if (IsByLink) {
            return LenCfg.GetMaxByLinkSnipLen() * MaxLenMultiplier;
        } else {
            return LenCfg.GetMaxSnipLen(parts) * MaxLenMultiplier;
        }
    }

    float TGetBestSnip::GetMaxPartLen()
    {
        return GetMaxSnipLen(0);
    }

    float TGetBestSnip::GetMaxLenForLPFactors()
    {
        return LenCfg.GetMaxSnipLen();
    }

    // Issue: SNIPPETS-1154
    bool TGetBestSnip::WantNFrag(int n) {
        if (n == 3) {
            return SentsMatchInfo.Query.UserPosCount() >= 4;
        }
        if (n == 4) {
            return SentsMatchInfo.Query.UserPosCount() >= 5;
        }
        return false;
    }
    bool TGetBestSnip::CutNFrags() {
        if (Cfg.VideoSnipAlgo()) {
            return true;
        }
        return SentsMatchInfo.Query.UserPosCount() > 64;
    }

    void TGetBestSnip::DoRedump() {
        if (!IsByLink) {
            NSnipRedump::GetRetexts(Cfg, WordSpanLen, SentsMatchInfo, Title, GetMaxLenForLPFactors(), TopCallback);
        }
    }
    void TGetBestSnip::DoAlgos(TOnSnip& onSnip) {
        Cfg.LogPerformance("DoAlgos.Start");
        TUniSpanIter spanSet(SentsMatchInfo, Restr, GrowRestr, WordSpanLen);
        if (!!Callback.GetDebugOutput()) {
            Callback.GetDebugOutput()->Print(false, "Current formula: %s", Cfg.GetAll().GetName().data());
        }
        TMxNetWeighter w(SentsMatchInfo, Cfg, WordSpanLen, !Cfg.TitWeight() ? nullptr : &Title, GetMaxLenForLPFactors(), Url, nullptr);
        if (Cfg.GetFormulaDegradeThreshold()) {
            w.InitFormulaDegrade(Cfg.GetFormulaDegradeThreshold(), ComputeHash(Url));
        }

        ECandidateSource source = IsByLink ? CS_LINK_ARC : CS_TEXT_ARC;
        const bool needCheckTitleForOrganicSnippets = Cfg.CheckPassageRepeatsTitleAlgo() != 0;

        if (1 <= GetMaxSnipCount()) {
            if (Cfg.VideoSnipAlgo()) {
                onSnip(NSnipVideoWordSpans::GetVideoSnippet(Cfg, w, spanSet, SentsMatchInfo, GetMaxSnipLen(), source, TopCallback, "Algo2"));
            } else {
                onSnip(NSnipWordSpans::GetTSnippet(w, spanSet, SentsMatchInfo, GetMaxSnipLen(1), source, TopCallback, nullptr, "Algo2", needCheckTitleForOrganicSnippets, Cfg.GetPassageRepeatsTitlePessimizeFactor()));
                if (Cfg.ExpFlagOn("use_factsnip")) {
                    TWordSpanLen symbolSpanLen(TCutParams::Symbol());
                    TUniSpanIter symbolCutSpanSet(SentsMatchInfo, Restr, GrowRestr, symbolSpanLen);
                    TMxNetWeighter wForFactsnip(SentsMatchInfo, Cfg, WordSpanLen, !Cfg.TitWeight() ? nullptr : &Title, GetMaxLenForLPFactors(), Url, SchemaOrgViewer, true);
                    onSnip.OnOneFragmentSymbolCut(NSnipWordSpans::GetTSnippet(wForFactsnip, symbolCutSpanSet, SentsMatchInfo, Cfg.GetSymbolCutFragmentWidth(), source, TopCallback, TopFactSnipCandidatesCallback, "Algo2SymbolCut"));
                }
            }
            Cfg.LogPerformance("DoAlgos.Fragment_1");
        }
        if (CutNFrags()) {
            Cfg.LogPerformance("DoAlgos.Stop");
            return;
        }
        NSnipWordSpans::TGetTwoPlusSnip getSnip(Cfg, w, spanSet, SentsMatchInfo, source, TopCallback, DontGrow, needCheckTitleForOrganicSnippets, Cfg.GetPassageRepeatsTitlePessimizeFactor());
        if (2 <= GetMaxSnipCount()) {
            onSnip(getSnip.GetSnip(GetMaxSnipLen(), GetMaxPartLen(), 2, TVector<TDynBitMap>()));
            Cfg.LogPerformance("DoAlgos.Fragment_2");
        }

        for (int i = 3; i <= 4; ++i) {
            if (i <= GetMaxSnipCount() && WantNFrag(i)) {
                // collect bitset for every already build candidates
                // NOTE: these masks are |title, but some stuff in algo/two_span.cpp is not using title matches for alike bitsets
                TSnip snip = getSnip.GetSnip(GetMaxSnipLen(i), GetMaxSnipLen(i), i, onSnip.GetBitMasks());
                if (snip.Snips.empty()) {
                    break;
                }
                onSnip(snip);
                Cfg.LogPerformance("DoAlgos.Fragment_3_4");
            }
        }

        Cfg.LogPerformance("DoAlgos.Stop");
    }

    TGetBestSnip::TGetBestSnip(
            const TSentsMatchInfo& sentsMatchInfo
            , const TSnipTitle& unnaturalTitle
            , const TConfig& cfg
            , const TString& url
            , const TLengthChooser& lenCfg
            , const TWordSpanLen& wordSpanLen
            , ISnippetsCallback& callback
            , TTopCandidateCallback* fsCallback
            , bool isByLink
            , float maxLenMultiplier
            , bool dontGrow
            , const TSchemaOrgArchiveViewer* schemaOrgViewer
    )
            : SentsMatchInfo(sentsMatchInfo)
            , Title(unnaturalTitle)
            , Cfg(cfg)
            , Url(url)
            , LenCfg(lenCfg)
            , WordSpanLen(wordSpanLen)
            , Callback(callback)
            , FactSnipCandidatesCallback(fsCallback)
            , IsByLink(isByLink)
            , MaxLenMultiplier(maxLenMultiplier)
            , DontGrow(dontGrow)
            , SchemaOrgViewer(schemaOrgViewer)
    {
        TopCallback = Callback.GetCandidateHandler();
        if (FactSnipCandidatesCallback) {
            TopFactSnipCandidatesCallback = FactSnipCandidatesCallback->GetCandidateHandler();
        } else {
            TopFactSnipCandidatesCallback = nullptr;
        }
        FillRestrictions();
    }


    TSnip TGetBestSnip::GetSnip(TSnip* oneFragmentSnip)
    {
        Cfg.LogPerformance("GetBestSnip.Start");

        DoRedump();

        TOnSnip onSnip(Title);

        DoAlgos(onSnip);

        TSnip res = onSnip.GetBestSnip();
        if (oneFragmentSnip != nullptr) {
            *oneFragmentSnip = onSnip.GetBestOneFragmentSnip();
            if (Cfg.ExpFlagOn("factsnip_dssm") && !Cfg.ExpFlagOff("get_query")) {
                AdjustCandidatesDssm(*oneFragmentSnip, Cfg, SentsMatchInfo, Callback);
            }
        }

        Cfg.LogPerformance("GetBestSnip.GetBest");
        Callback.OnBestFinal(res, IsByLink);
        if (FactSnipCandidatesCallback) {
            FactSnipCandidatesCallback->OnBestFinal(res, IsByLink);
        }
        Cfg.LogPerformance("GetBestSnip.Stop");
        return res;
    }
}
