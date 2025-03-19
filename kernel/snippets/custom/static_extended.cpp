#include "static_extended.h"

#include "add_extended.h"
#include "extended_length.h"

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/sent_match/meta.h>
#include <kernel/snippets/static_annotation/static_annotation.h>

namespace NSnippets {

    static TUtf16String GetMetaExtension(const TReplaceContext& context) {
        return context.MetaDescr.GetTextCopy();
    }

    static TUtf16String GetStaticAnnotExtension(const TReplaceContext& context, const TStatAnnotViewer& statAnnotViewer) {
        if (statAnnotViewer.GetResultType() != EStatAnnotType::SAT_MAIN_CONTENT) {
            return TUtf16String();
        }
        TStaticAnnotation staticAnnotation(context.Cfg, context.Markup);
        staticAnnotation.InitFromSentenceViewer(statAnnotViewer, context.SuperNaturalTitle, context.QueryCtx);
        return staticAnnotation.GetSpecsnippet();
    }

    void TStaticExtendedReplacer::DoWork(TReplaceManager* manager) {
        const TReplaceContext& ctx = manager->GetContext();
        if (!ctx.Cfg.IsTouchReport() && !ctx.Cfg.IsPadReport() && !ctx.Cfg.IsNeedExt()) {
            return;
        }
        if (ctx.Cfg.ExpFlagOff("extend_with_static")) {
            return;
        }
        TReplaceResult replaceResult = manager->GetResult();
        const TString& headlineSrc = replaceResult.GetTextSrc();
        if (headlineSrc != "creativework_snip" &&
            headlineSrc != "encyc" &&
            headlineSrc != "yaca")
        {
            return;
        }
        const TMultiCutResult& extDesc = replaceResult.GetTextExt();
        if (extDesc.Long.size() > extDesc.Short.size()) {
            return;
        }
        TMultiCutResult staticExtDesc;
        staticExtDesc.Short = extDesc.Short;
        staticExtDesc.Long = extDesc.Short;
        TUtf16String extension = GetStaticAnnotExtension(ctx, StatAnnotViewer);
        const float maxExtLen = GetExtSnipLen(ctx.Cfg, ctx.LenCfg);
        if (!CheckAndAddExtension(extension, maxExtLen, ctx, staticExtDesc, ctx.DocLangId)) {
            extension = GetMetaExtension(ctx);
            CheckAndAddExtension(extension, maxExtLen, ctx, staticExtDesc, ctx.DocLangId);
        }
        if (staticExtDesc.Long.size() > staticExtDesc.Short.size()) {
            replaceResult.UseText(staticExtDesc, headlineSrc);
            manager->Commit(replaceResult);
        }
    }
}
