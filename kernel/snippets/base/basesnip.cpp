#include "basesnip.h"

#include "basesnip_viewers.h"
#include "basesnip_impl.h"
#include "raw.h"
#include "pseudo_rand.h"

#include <util/random/mersenne.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <util/string/subst.h>

namespace NSnippets {

static void AddCommonMarkers(TPassageReply& reply, const TConfig& cfg)
{
    const TString snipDebug = cfg.GetSnipParamsForDebug();
    if (snipDebug) {
        reply.AddMarker("SnipDebug=" + snipDebug);
    }
    reply.AddMarker("SnipLenStr=" + ToString(reply.GetSnipLengthInRows()));
    reply.AddMarker("SnipLenSym=" + ToString(reply.GetSnipLengthInSymbols()));
    if (reply.GetPassages() && reply.GetPassagesType() == 1) {
        reply.AddMarker("SnipByLink=1");
        if (reply.GetHeadline().empty()) {
            reply.AddMarker("SnipByLinkOnly=1");
        }
    }
}

static void DropPreviewJsonIfNeeded(TPassageReply& reply, const TConfig& cfg)
{
    if (!cfg.ForceStann()) {
        reply.SetPreviewJson(TString());
        reply.SetRawPreview(NProto::TRawPreview());
    }
}

void FillPassageReply(TPassageReply& reply, const TPassageReplyContext& replyContext)
{
    const TConfig cfg(replyContext.ConfigParams);
    try {
        if (cfg.GetRawPassages() || cfg.IsPaintedRawPassages() || cfg.RawAll()) {
            cfg.LogPerformance("FillReply.Start");
            ProcessRawPassages(reply, cfg, replyContext.HitsInfo, replyContext.IH, replyContext.ArcCtx);
            cfg.LogPerformance("FillReply.ProcessRawPassages");
            cfg.LogPerformance("FillReply.Stop");
            return;
        }

        const TString url = replyContext.ArcCtx.GetTextArc().GetDescr().get_url();

        TQueryy queryCtx(replyContext.Richtree.Get() ? replyContext.Richtree->Root.Get() : nullptr, cfg);
        if (!cfg.IgnoreRegionPhrase() && replyContext.RegionPhraseRichtree.Get() && replyContext.RegionPhraseRichtree->Root.Get()) {
            queryCtx.RegionQuery.Reset(new TRegionQuery(replyContext.RegionPhraseRichtree->Root.Get()));
        }

        if (cfg.AltSnippetCount()) {
            NSnippets::NProto::TSecondarySnippets ss;

            TConfigParams cfgp0 = replyContext.ConfigParams;
            cfgp0.AppendExps.push_back(cfg.AltSnippetExps(0));
            const TConfig cfg0(cfgp0);
            TFillPassageReply fprprod(reply, cfg0, replyContext.HitsInfo, replyContext.IH, replyContext.ArcCtx, queryCtx, replyContext.RichtreeWanderer);
            fprprod.FillReply();

            for (int i = 1; i <= cfg.AltSnippetCount(); i++) {
                TPassageReply extraReply;
                TConfigParams extraCfgp = replyContext.ConfigParams;
                extraCfgp.AppendExps.push_back(cfg.AltSnippetExps(i));
                const TConfig extraCfg(extraCfgp);
                TFillPassageReply extraFpr(extraReply, extraCfg, replyContext.HitsInfo, replyContext.IH, replyContext.ArcCtx, queryCtx, replyContext.RichtreeWanderer);
                extraFpr.FillReply();
                if (cfg.ProdLinesCount() && ceil(reply.GetSnipLengthInRows()) != ceil(extraReply.GetSnipLengthInRows())) {
                    extraReply = reply;
                    AddCommonMarkers(extraReply, cfg0);
                    extraReply.AddMarker("SnipExp=MultiSnip0");
                    DropPreviewJsonIfNeeded(extraReply, cfg0);
                } else {
                    AddCommonMarkers(extraReply, extraCfg);
                    extraReply.AddMarker(Sprintf("SnipExp=MultiSnip%d", i));
                    DropPreviewJsonIfNeeded(extraReply, extraCfg);
                }
                NMetaProtocol::TArchiveInfo* ai = ss.AddSnippets();
                extraReply.PackToArchiveInfo(*ai);
            }

            AddCommonMarkers(reply, cfg0);
            reply.AddMarker("SnipExp=MultiSnip0");
            reply.PackAltSnippets(ss);
            DropPreviewJsonIfNeeded(reply, cfg);
        }
        else if (cfg.UseSnipExpMarkers()) {
            TPassageReply prod;
            TFillPassageReply fprprod(prod, cfg, replyContext.HitsInfo, replyContext.IH, replyContext.ArcCtx, queryCtx, replyContext.RichtreeWanderer);
            fprprod.FillReply();
            TString expsForMarkers = cfg.GetExpsForMarkers();
            TConfigParams cfgp2 = replyContext.ConfigParams;
            cfgp2.AppendExps.push_back(expsForMarkers);
            const TConfig cfg2(cfgp2);
            TFillPassageReply fprexp(reply, cfg2, replyContext.HitsInfo, replyContext.IH, replyContext.ArcCtx, queryCtx, replyContext.RichtreeWanderer);
            fprexp.FillReply();
            bool titlesDiff = prod.GetTitle() != reply.GetTitle();
            bool titlesFreeDiff = prod.GetPassages() != reply.GetPassages() ||
                                 prod.GetHeadline() != reply.GetHeadline() ||
                                 prod.GetHeadlineSrc() != reply.GetHeadlineSrc() ||
                                 prod.GetPreviewJson() != reply.GetPreviewJson() ||
                                 prod.GetUrlMenu() != reply.GetUrlMenu() ||
                                 prod.GetHilitedUrl() != reply.GetHilitedUrl() ||
                                 prod.GetClickLikeSnip() != reply.GetClickLikeSnip();
            bool someDiff = false;
            if (cfg.TitlesDiffOnly()) {
                someDiff = titlesDiff && !titlesFreeDiff;
            } else {
                someDiff = titlesDiff || titlesFreeDiff;
            }
            float prodSnipLen = prod.GetSnipLengthInRows();
            float replySnipLen = reply.GetSnipLengthInRows();
            if (cfg.ChooseLongestSnippet()) {
                someDiff = (replySnipLen > prodSnipLen);
            }
            size_t pseudoRand = PseudoRand(url, replyContext.HitsInfo);
            TMersenne<ui32> mersenn(pseudoRand);
            if (someDiff) {
                int randomNumber = mersenn.GenRand() % 100;
                if (randomNumber >= cfg.GetDiffRatio())
                    someDiff = false;
                if (cfg.ProdLinesCount() && ceil(prodSnipLen) != ceil(replySnipLen))
                    someDiff = false;
            }
            if (!someDiff) {
                reply = prod;
            } else {
                bool shouldSwap = cfg.SwapWithProd();
                if (mersenn.GenRand() & 1)
                    shouldSwap = false;
                if (cfg.SnipExpProd() != shouldSwap) {
                    reply = prod;
                }
                SubstGlobal(expsForMarkers, ',', '#');
                reply.AddMarker("SnipExp=" + expsForMarkers);
            }
            AddCommonMarkers(reply, cfg);
            DropPreviewJsonIfNeeded(reply, cfg);
        } else {
            TFillPassageReply fpr(reply, cfg, replyContext.HitsInfo, replyContext.IH, replyContext.ArcCtx, queryCtx, replyContext.RichtreeWanderer);
            fpr.FillReply();
            AddCommonMarkers(reply, cfg);
            DropPreviewJsonIfNeeded(reply, cfg);
        }
    } catch (...) {
        reply.SetError(CurrentExceptionMessage());
    }
}

}
