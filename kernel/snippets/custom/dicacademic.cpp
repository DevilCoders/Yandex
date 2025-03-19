#include "dicacademic.h"

#include <kernel/snippets/cut/cut.h>
#include <kernel/snippets/sent_match/len_chooser.h>
#include <kernel/snippets/sent_match/meta.h>

namespace NSnippets {
    static const char* const HOST = "dic.academic.ru/";

    void TDicAcademicReplacer::DoWork(TReplaceManager* manager) {
        const TReplaceContext& ctx = manager->GetContext();
        if (!ctx.MetaDescr.MayUse()) {
            return;
        }
        if (!ctx.Url.StartsWith(HOST)) {
            return;
        }
        TUtf16String snip = ctx.MetaDescr.GetTextCopy();
        SmartCut(snip, ctx.IH, ctx.LenCfg.GetMaxSpecSnipLen(), TSmartCutOptions(ctx.Cfg));
        manager->Commit(TReplaceResult().UseText(snip, "dicacademic"), MRK_DIC_ACADEMIC);
    }
}
