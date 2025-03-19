#include "raw.h"
#include <kernel/snippets/sent_match/glue.h>

#include <kernel/snippets/archive/markup/markup.h>
#include <kernel/snippets/archive/unpacker/unpacker.h>
#include <kernel/snippets/archive/view/order.h>
#include <kernel/snippets/archive/view/view.h>
#include <kernel/snippets/archive/viewers.h>
#include <kernel/snippets/archive/view/storage.h>

#include <kernel/snippets/config/config.h>

#include <kernel/snippets/iface/archive/manip.h>
#include <kernel/snippets/iface/passagereply.h>

#include <kernel/snippets/strhl/goodwrds.h>
#include <kernel/snippets/strhl/glue_common.h>
#include <kernel/snippets/strhl/zonedstring.h>

#include <library/cpp/charset/wide.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NSnippets {

extern const TUtf16String EV_NOTITLE_WIDE;
extern const TUtf16String EV_NOHEADLINE_WIDE;

void ProcessRawPassages(TPassageReply& reply,
    const TConfig& cfg,
    const THitsInfoPtr& hitsInfo,
    const TInlineHighlighter& IH,
    TArcManip& arcCtx)
{
    TSentsOrder order;
    const bool link = hitsInfo.Get() && hitsInfo->IsGoogle();
    if (cfg.RawAll()) {
        order.PushBack(1, 65536);
    } else {
        if (hitsInfo.Get()) {
            for (ui16 hit : (link ? hitsInfo->LinkHits : hitsInfo->THSents)) {
                order.PushBack(hit, hit);
            }
        }
    }

    TArchiveStorage tempStorage;
    TArchiveMarkup markup;
    TUnpacker unpacker(cfg, &tempStorage, &markup, link ? ARC_LINK : ARC_TEXT);
    TMetadataViewer mdata(false);
    unpacker.AddRequester(&mdata);
    unpacker.AddRequest(order);

    const int res = arcCtx.GetUnpText(unpacker, link);
    TUtf16String title = EV_NOTITLE_WIDE;
    TUtf16String headline = EV_NOHEADLINE_WIDE;
    TVector<TZonedString> snipVec;
    TVector<TString> attrVec;
    if (res == 0) {
        TVector<TUtf16String> titles;
        DumpResultCopy(mdata.Title, titles);
        title = GlueTitle(titles);

        TVector<TUtf16String> headlines;
        DumpResultCopy(mdata.Meta, headlines);
        headline = GlueHeadline(headlines);

        TArchiveView vtext;
        DumpResult(order, vtext);
        const int cnt = cfg.RawAll() ? (int)vtext.Size() : Min(cfg.GetMaxSnipCount(), (int)vtext.Size());
        snipVec.resize(cnt);
        attrVec.resize(cnt);
        for (int i = 0; i < cnt; ++i) {
            const TArchiveSent* sent = vtext.Get(i);
            snipVec[i] = TZonedString(ToWtring(sent->Sent));
            attrVec[i] = WideToChar(sent->Attr, CODES_YANDEX);
        }
    }

    IH.PaintPassages(title);
    IH.PaintPassages(headline);
    if (cfg.IsPaintedRawPassages()) {
        IH.PaintPassages(snipVec);
    }

    TPassageReplyData data;
    data.LinkSnippet = link;
    data.Passages = MergedGlue(snipVec);
    data.Attrs = attrVec;
    data.Title = title;
    data.Headline = headline;
    reply.Set(data);
}

}
