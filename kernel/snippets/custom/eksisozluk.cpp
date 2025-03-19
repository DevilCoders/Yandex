#include "eksisozluk.h"
#include "listsnip.h"

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/custom/schemaorg_viewer/schemaorg_viewer.h>
#include <kernel/snippets/cut/cut.h>
#include <kernel/snippets/iface/archive/sent.h>
#include <kernel/snippets/schemaorg/sozluk_comments.h>
#include <kernel/snippets/sent_match/len_chooser.h>
#include <kernel/snippets/sent_match/retained_info.h>
#include <kernel/snippets/sent_match/tsnip.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NSnippets {

static const char* const HEADLINE_SRC_LIST = "list_snip";
static const char* const HEADLINE_SRC_SINGLE = "eksisozluk_snip";
static const char* const LIST_DATA = "listData";

static const size_t FIRST_COMMENTS_USE = 3;
static const size_t MIN_SNIP_LEN = 50;

TEksisozlukReplacer::TEksisozlukReplacer(const TSchemaOrgArchiveViewer& arcViewer)
    : IReplacer("eksisozluk")
    , ArcViewer(arcViewer)
{
}

void TEksisozlukReplacer::DoWork(TReplaceManager* manager) {
    const TReplaceContext& ctx = manager->GetContext();

    const NSchemaOrg::TSozlukComments* data = ArcViewer.GetEksisozlukComments();
    if (!data) {
        return;
    }
    if (ctx.Cfg.IsMainPage()) {
        manager->ReplacerDebug("the main page is not processed");
        return;
    }
    TVector<TUtf16String> sents = data->GetCleanComments(FIRST_COMMENTS_USE);
    if (sents.empty()) {
        manager->ReplacerDebug("UserComments/commentText fields are missing or empty");
        return;
    }

    bool useList = sents.size() > 1;
    float cutLength = (useList ? 0.95f * ctx.LenCfg.GetRowLen() : ctx.LenCfg.GetNDesktopRowsLen(2));
    TSmartCutOptions options(ctx.Cfg);
    options.AddEllipsisToShortText = false;
    for (TUtf16String& sent : sents) {
        SmartCut(sent, ctx.IH, cutLength, options);
    }

    TRetainedSentsMatchInfo& customSents = manager->GetCustomSnippets().CreateRetainedInfo();
    customSents.SetView(sents, TRetainedSentsMatchInfo::TParams(ctx.Cfg, ctx.QueryCtx));
    TSnip snip = customSents.AllAsSnip();

    TReplaceResult res;
    res.UseSnip(snip, useList ? HEADLINE_SRC_LIST : HEADLINE_SRC_SINGLE);

    size_t snipLength = snip.GetRawTextWithEllipsis().size();
    if (snipLength < MIN_SNIP_LEN) {
        manager->ReplacerDebug("too short", res);
        return;
    }

    if (useList) {
        TListSnipData listData;
        listData.ItemCount = static_cast<int>(data->Comments.size());
        listData.First = 0;
        listData.Last = static_cast<int>(snip.Snips.size()) - 1;
        TString listValue = listData.Dump(ctx.Cfg.DropListStat());
        res.AppendSpecSnipAttr(LIST_DATA, listValue);
    }
    manager->Commit(res);
}

} // namespace NSnippets
