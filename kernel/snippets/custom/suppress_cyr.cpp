#include "suppress_cyr.h"

#include <kernel/snippets/archive/view/view.h>

#include <kernel/snippets/qtree/query.h>

#include <kernel/snippets/sent_info/sent_info.h>

#include <kernel/snippets/sent_match/lang_check.h>
#include <kernel/snippets/sent_match/restr.h>
#include <kernel/snippets/sent_match/sent_match.h>
#include <kernel/snippets/sent_match/tsnip.h>

#include <kernel/snippets/titles/make_title/util_title.h>

#include <kernel/snippets/wordstat/wordstat.h>

namespace NSnippets {

void TCyrillicToEmptyReplacer::DoWork(TReplaceManager* manager) {
    const TReplaceContext& repCtx = manager->GetContext();

    if (repCtx.QueryCtx.CyrillicQuery) {
        return;
    }

    TSnip snip(repCtx.Snip);
    TUtf16String headline;

    TReplaceResult result;

    if (manager->IsReplaced()) {
        result = manager->GetResult();
        const TString& src = result.GetTextSrc();
        if (src == "mediawiki_snip") {
            return;
        }
        if (result.GetSnip()) {
            snip = *result.GetSnip();
        }
        if (result.CanUse()) {
            headline = result.GetText();
        }
    }
    else {
        result.UseTitle(repCtx.SuperNaturalTitle);
    }

    bool commit = false;
    if (HasTooManyCyrillicWords(result.GetTitle()->GetTitleString(), 1)) {
        if (HasTooManyCyrillicWords(repCtx.NaturalTitle.GetTitleString(), 1)) { // we have nothing to do...
            result.UseTitle(TSnipTitle());
        } else { // use natural title instead!
            result.UseTitle(repCtx.NaturalTitle);
        }
        commit = true;
    }

    bool cyrHeadline = HasTooManyCyrillicWords(headline, 2);
    bool cyrSnip = HasTooManyCyrillicWords(snip.GetRawTextWithEllipsis(), 2);

    if (headline && !cyrHeadline && cyrSnip && repCtx.IsByLink) {
        result.UseSnip(TSnip(), result.GetTextSrc());
        commit = true;
    }
    else if (cyrHeadline || cyrSnip) {
        result.UseText(TUtf16String(), "empty");
        result.ClearSpecSnipAttrs();
        commit = true;
    }

    if (commit) {
        manager->Commit(result, MRK_SUPPRESS_CYRILLIC);
    }
}

}
