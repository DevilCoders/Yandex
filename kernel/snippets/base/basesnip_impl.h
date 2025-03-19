#pragma once

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/qtree/query.h>
#include <kernel/snippets/hits/ctx.h>
#include <kernel/snippets/iface/archive/manip.h>
#include <kernel/snippets/iface/passagereply.h>

#include <kernel/snippets/strhl/goodwrds.h>
#include <kernel/snippets/strhl/zonedstring.h>
#include <kernel/snippets/urlcut/url_wanderer.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/ptr.h>
#include <util/digest/numeric.h>

namespace NSnippets {

class TFillPassageReply {
public:
    struct IImpl {
        virtual void FillReply() = 0;
        virtual ~IImpl() {
        }
    };

private:
    THolder<IImpl> Impl;

public:
    TFillPassageReply(TPassageReply& reply,
        const TConfig& cfg,
        const THitsInfoPtr hitsInfo,
        const TInlineHighlighter& ih,
        TArcManip& arcctx,
        const TQueryy& queryCtx,
        NUrlCutter::TRichTreeWanderer& richtreeWanderer);

    ~TFillPassageReply();

    void FillReply();
};
}
