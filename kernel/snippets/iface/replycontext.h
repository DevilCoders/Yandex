#pragma once

#include "configparams.h"
#include "passagereply.h"
#include <kernel/snippets/iface/archive/manip.h>

#include <kernel/snippets/hits/ctx.h>

#include <kernel/snippets/urlcut/url_wanderer.h>
#include <kernel/snippets/strhl/goodwrds.h>
#include <kernel/qtree/richrequest/richnode.h>


namespace NSnippets {

class TPassageReplyContext {
public:
    const TConfigParams& ConfigParams;
    const THitsInfoPtr HitsInfo;
    const TInlineHighlighter& IH;
    TArcManip& ArcCtx;
    TRichTreeConstPtr Richtree;
    TRichTreeConstPtr RegionPhraseRichtree;
    NUrlCutter::TRichTreeWanderer& RichtreeWanderer;

public:
    TPassageReplyContext(
        const TConfigParams& cfgp,
        const THitsInfoPtr& hitsInfo,
        const TInlineHighlighter& ih,
        TArcManip& arcCtx,
        const TRichTreeConstPtr& richtree,
        const TRichTreeConstPtr& regionPhraseRichtree,
        NUrlCutter::TRichTreeWanderer& richtreeWanderer
    )
      : ConfigParams(cfgp)
      , HitsInfo(hitsInfo)
      , IH(ih)
      , ArcCtx(arcCtx)
      , Richtree(richtree)
      , RegionPhraseRichtree(regionPhraseRichtree)
      , RichtreeWanderer(richtreeWanderer)
    {
    }
};

}
