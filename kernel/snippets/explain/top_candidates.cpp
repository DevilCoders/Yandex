#include "top_candidates.h"

#include <kernel/snippets/sent_match/enhance.h>
#include <kernel/snippets/sent_match/extra_attrs.h>
#include <kernel/snippets/sent_match/glue.h>
#include <kernel/snippets/sent_match/sent_match.h>
#include <kernel/snippets/sent_match/similarity.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/algo/redump.h>

#include <kernel/snippets/explain/finaldumpbin.h>
#include <kernel/snippets/explain/pooldump.h>

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/formulae/formula.h>
#include <kernel/snippets/iface/archive/sent.h>
#include <kernel/snippets/iface/passagereply.h>

#include <kernel/snippets/sent_info/sent_info.h>

#include <kernel/snippets/strhl/glue_common.h>
#include <kernel/snippets/strhl/zonedstring.h>

#include <library/cpp/scheme/scheme.h>

#include <util/generic/algorithm.h>
#include <util/generic/map.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NSnippets
{
    struct TCandidates : IAlgoTop {
        TVector<TSnip> Snippets;

        void Push(const TSnip& snip, const TUtf16String& /*title = TUtf16String()*/) override {
            Snippets.emplace_back(snip);
        }
        void Push(const TSnip& snip) {
            Snippets.emplace_back(snip);
        }
    };

    class TTopCandidateCallback::TImpl : public ISnippetCandidateDebugHandler {
    private:
        const TConfig& Cfg;
        TExtraSnipAttrs& ExtraSnipAttrs;
        const TInlineHighlighter& IH;
        TCandidates AllCandidates;
        TCandidates MetadataCandidates;
        TCandidates ReadyTopCandidates;
        TString Explanation;
        TString SerializedTitle;
        bool ExplainMode;

    public:
        TImpl(const TConfig& cfg, TExtraSnipAttrs& extraSnipAttrs, const TInlineHighlighter& ih, bool explainMode)
            : Cfg(cfg)
            , ExtraSnipAttrs(extraSnipAttrs)
            , IH(ih)
            , ExplainMode(explainMode)
        {
        }

        IAlgoTop* AddTop(const char* algo, ECandidateSource src) override {
            bool sourceGood = !ExplainMode || (src == CS_TEXT_ARC || Cfg.IsDumpForPoolGeneration() && src == CS_METADATA);
            if (!sourceGood || Cfg.SkipAlgoInTopCandidateDump(algo)) {
                return nullptr;
            }
            return src == CS_METADATA ? &MetadataCandidates : &AllCandidates;
        }

        void SaveTitle(const TSnip& title) {
            if (Cfg.IsDumpForPoolGeneration()) {
                SerializedTitle = NSnipRedump::SerializeCustomSource(title);
            }
        }


        bool CandidatesQueueFull(const NSc::TValue& topCandidates) const {
            return( (ExplainMode && (topCandidates.ArraySize() >= Cfg.TopCandidateCount())) || (!ExplainMode && (ReadyTopCandidates.Snippets.size() >= Cfg.FactSnipTopCandidateCount())));
        }

        void FillTopCandidates(const TPassageReply* reply, const TEnhanceSnippetConfig* enhanceCfg) {
            if (ExplainMode && ( Cfg.IsDumpForPoolGeneration() && !reply || !Cfg.IsDumpForPoolGeneration() && reply )) {
                return;
            }

            TVector<TSnip>& snips = AllCandidates.Snippets;
            StableSortBy(snips, [](const TSnip& snip) { return -snip.Weight; });

            NSc::TValue topCandidates;
            topCandidates.SetArray();
            TMap<int, int> usedSpans;
            TVector<TEQInfo> usedSnips;
            const bool allowIntersection = Cfg.ExpFlagOn("top_candidate_allow_intersection");
            for (const TSnip& snip : snips) {
                if (CandidatesQueueFull(topCandidates)) {
                    break;
                }
                if (allowIntersection || !HasIntersection(snip, usedSpans)) {
                    if (Cfg.IsDumpForPoolGeneration() && !allowIntersection) {
                        TEQInfo eqInfo(snip);
                        if (HasBagOfWordsIntersection(eqInfo, usedSnips, Cfg.BagOfWordsIntersectionThreshold())) {
                            continue;
                        }
                        usedSnips.emplace_back(std::move(eqInfo));
                    }
                    UpdateSpans(snip, usedSpans);
                    if (!ExplainMode) {
                        ReadyTopCandidates.Push(snip);
                    } else {
                        PushCandidate(snip, topCandidates, reply, enhanceCfg, CS_TEXT_ARC);
                    }
                }
            }
            StableSortBy(MetadataCandidates.Snippets, [](const TSnip& snip) { return -snip.Weight; });
            for (const TSnip& snip : MetadataCandidates.Snippets) {
                if (CandidatesQueueFull(topCandidates)) {
                    break;
                }
                // no spans for custom snippets
                if (Cfg.IsDumpForPoolGeneration() && !allowIntersection) {
                    TEQInfo eqInfo(snip);
                    if (HasBagOfWordsIntersection(snip, usedSnips, Cfg.BagOfWordsIntersectionThreshold())) {
                        continue;
                    }
                    usedSnips.push_back(std::move(eqInfo));
                }
                if (!ExplainMode) {
                        ReadyTopCandidates.Push(snip);
                    } else {
                        PushCandidate(snip, topCandidates, reply, enhanceCfg, CS_METADATA);
                }
            }
            if (ExplainMode) {
                if (Cfg.IsDumpForPoolGeneration()) {
                    // it's too late to set ExtraSnipAttrs from within OnPassageReply()
                    Explanation = topCandidates.ToJsonSafe();
                } else {
                    ExtraSnipAttrs.AddClickLikeSnipJson("top_candidates", topCandidates.ToJsonSafe());
                }
            }
        }

        const TString& GetExplanation() const {
            return Explanation;
        }

        TVector<TSnip>& GetTopCandidates() {
            return ReadyTopCandidates.Snippets;
        }

    private:
        static bool HasIntersection(const TSnip& snip, const TMap<int, int>& usedSpans) {
            for (const TSingleSnip& singleSnip : snip.Snips) {
                // check nearest span right from the current span begin
                auto neighbour = usedSpans.lower_bound(singleSnip.GetFirstWord());
                if (neighbour != usedSpans.end() && neighbour->first < singleSnip.GetLastWord()) {
                    return true;
                }
                // check nearest span left from the current span begin
                if (neighbour != usedSpans.begin()) {
                    --neighbour;
                    if (neighbour->second > singleSnip.GetFirstWord()) {
                        return true;
                    }
                }
            }
            return false;
        }

        static bool HasBagOfWordsIntersection(const TEQInfo& eqInfo, const TVector<TEQInfo>& usedSnips, double threshold) {
            if (!eqInfo.Total) {
                return true; // we don't want empty snippets
            }
            for (const TEQInfo& otherSnip : usedSnips) {
                size_t matched = eqInfo.CountEqWords(otherSnip);
                double sim = (double)matched / (eqInfo.Total + otherSnip.Total - matched);
                if (sim >= threshold) {
                    return true;
                }
            }
            return false;
        }

        static void UpdateSpans(const TSnip& snip, TMap<int, int>& usedSpans) {
            for (const TSingleSnip& singleSnip : snip.Snips) {
                usedSpans.emplace(singleSnip.GetFirstWord(), singleSnip.GetLastWord());
            }
        }

        void PushCandidate(const TSnip& snip, NSc::TValue& topCandidates, const TPassageReply* reply, const TEnhanceSnippetConfig* enhanceCfg, ECandidateSource src) const {
            NSc::TValue item;
            if (Cfg.IsDumpForPoolGeneration()) {
                Y_ASSERT(reply);
                Y_ASSERT(enhanceCfg);
                TVector<TZonedString> snipVec = snip.GlueToZonedVec(false);
                TExtraSnipAttrs linkAttrs;
                if (!Cfg.ExpFlagOn("top_candidate_no_postprocess"))
                    EnhanceSnippet(*enhanceCfg, snipVec, linkAttrs);

                TPaintingOptions paintingOptions = TPaintingOptions::DefaultSnippetOptions();
                paintingOptions.SkipAttrs = Cfg.PaintNoAttrs();
                paintingOptions.Fred = Cfg.Fred();
                IH.PaintPassages(snipVec, paintingOptions);

                // "reply" attribute is for SNIPPETS-6743,
                // ability to substitute the candidate snippet as if it was returned by the basesearch
                TPassageReplyData replyData;
                replyData.LinkSnippet = false;
                replyData.Passages = MergedGlue(snipVec);
                replyData.Title = reply->GetTitle();
                replyData.DocQuerySig = reply->GetDocQuerySig();
                replyData.DocStaticSig = reply->GetDocStaticSig();
                replyData.UrlMenu = reply->GetUrlMenu();
                replyData.HilitedUrl = reply->GetHilitedUrl();

                TPassageReply extraReply;
                extraReply.Set(replyData);
                NMetaProtocol::TArchiveInfo ai;
                extraReply.PackToArchiveInfo(ai);
                TString binProto;
                Y_PROTOBUF_SUPPRESS_NODISCARD ai.SerializeToString(&binProto);
                item["reply"] = Base64EncodeUrl(binProto);

                // "redump" attribute is for SNIPPETS-6738,
                // ability to recalculate all factors even if the candidate is not generated anymore
                TString redump;
                if (src == CS_METADATA) {
                    redump = NSnipRedump::SerializeCustomSource(snip);
                } else {
                    TArcFragments fragments = NSnipRedump::SnipToArcFragments(snip, true);
                    redump = fragments.Save();
                }
                redump += '|';
                redump += SerializedTitle;
                item["redump"] = redump;

                // "passages" attribute is not used by the conveyor,
                // it's for human-readable identification
                // ("reply" already has them, but in inconvenient form)
                item["passages"].SetArray();
                for (const TUtf16String& passage : replyData.Passages) {
                    item["passages"].Push(NSc::TValue(WideToUTF8(passage)));
                }

                item["slices"] = SerializeFactorBorders(Cfg.GetTotalFormula().GetFactorDomain().GetBorders(), NFactorSlices::ESerializationMode::LeafOnly);
            } else {
                Y_ASSERT(!reply);
                Y_ASSERT(!enhanceCfg);
                TPaintingOptions paintingOptions = TPaintingOptions::DefaultSnippetOptions();
                paintingOptions.SkipAttrs = Cfg.PaintNoAttrs();
                paintingOptions.Fred = Cfg.Fred();
                TVector<TZonedString> snipVec = snip.GlueToZonedVec(false);
                IH.PaintPassages(snipVec, paintingOptions);

                for (const TUtf16String& passage : MergedGlue(snipVec)) {
                    item["passages"].Push(NSc::TValue(WideToUTF8(passage)));
                }
            }
            if (Cfg.ExpFlagOn("add_top_candidate_factors")) {
                TFactorsToDump in = Cfg.TopCandidateFactorsToDump();
                if (Cfg.IsDumpForPoolGeneration()) {
                    in.DumpFormulaWeight = false;
                    item["weight"] = snip.Weight;
                }
                if (TString factorsDump = TFinalFactorsDumpBinary::DumpFactors(snip, in)) {
                    item["factors"] = factorsDump;
                }
            }
            topCandidates.SetArray().Push(item);
        }
    };

    TTopCandidateCallback::TTopCandidateCallback(const TConfig& cfg, TExtraSnipAttrs& extraSnipAttrs, const TInlineHighlighter& ih, bool silentMode)
      : Impl(new TImpl(cfg, extraSnipAttrs, ih, silentMode))
    {}

    TTopCandidateCallback::~TTopCandidateCallback()
    {}

    ISnippetCandidateDebugHandler* TTopCandidateCallback::GetCandidateHandler() {
        return Impl.Get();
    }

    void TTopCandidateCallback::OnTitleSnip(const TSnip& /*natural*/, const TSnip& unnatural, bool /*isByLink*/) {
        Impl->SaveTitle(unnatural);
    }

    void TTopCandidateCallback::OnBestFinal(const TSnip& /*snip*/, bool /*isByLink*/) {
        Impl->FillTopCandidates(nullptr, nullptr);
    }

    void TTopCandidateCallback::OnPassageReply(const TPassageReply& reply, const TEnhanceSnippetConfig& cfg) {
        Impl->FillTopCandidates(&reply, &cfg);
    }

    void TTopCandidateCallback::GetExplanation(IOutputStream& expl) const {
        expl << Impl->GetExplanation();
    }
    TVector<TSnip>& TTopCandidateCallback::GetBestCandidates() {
        return Impl->GetTopCandidates();
    }

}
