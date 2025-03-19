#include "extended_length.h"

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/sent_match/glue.h>
#include <kernel/snippets/sent_match/len_chooser.h>
#include <kernel/snippets/sent_match/restr.h>
#include <kernel/snippets/sent_match/extra_attrs.h>
#include <kernel/snippets/sent_match/sent_match.h>
#include <kernel/snippets/algo/extend.h>
#include <kernel/snippets/sent_match/callback.h>
#include <kernel/snippets/smartcut/pixel_length.h>

#include <kernel/snippets/strhl/hilite_mark.h>
#include <kernel/snippets/strhl/zonedstring.h>

#include <util/charset/wide.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/string/subst.h>

namespace NSnippets {

static const TUtf16String ELLIPSIS_STR = u"...";
static const TUtf16String ELLIPSIS_CHAR = u"…";

// Check for badly cut descriptions
static bool HasPartialLastWord(const TUtf16String& desc) {
    return desc.size() >= 150 && desc.size() <= 210 &&
        desc.EndsWith(ELLIPSIS_STR) && IsAlnum(desc[desc.size() - ELLIPSIS_STR.size() - 1]);
}

TMultiCutResult CutDescription(const TUtf16String& srcDesc, const TReplaceContext& ctx, float maxLen) {
    TSmartCutOptions options(ctx.Cfg);
    return CutDescription(srcDesc, ctx, maxLen, options);
}

TMultiCutResult CutDescription(const TUtf16String& srcDesc, const TReplaceContext& ctx, float maxLen, TSmartCutOptions& options) {
    TUtf16String desc = srcDesc;
    SubstGlobal(desc, u"&quot;", u"\"");
    Collapse(desc);
    Strip(desc);
    if (!desc) {
        return TMultiCutResult();
    }

    if (HasPartialLastWord(desc)) {
        options.CutLastWord = true;
    }
    if (desc.EndsWith(ELLIPSIS_STR)) {
        desc.erase(desc.size() - ELLIPSIS_STR.size());
    }
    if (desc.EndsWith(ELLIPSIS_CHAR)) {
        desc.erase(desc.size() - ELLIPSIS_CHAR.size());
    }
    float maxExtLen = GetExtSnipLen(ctx.Cfg, ctx.LenCfg);
    if (ctx.Cfg.ExpFlagOn("no_extsnip")) {
        maxExtLen = 0;
    }
    return SmartCutWithExt(desc, ctx.IH, maxLen, maxExtLen, options);
}

static struct { ELanguage Lang; TUtf16String ReadMoreText; } HIDE_TEXTS[] = {
    {LANG_ENG, u"Hide"}, // The first value is a default
    {LANG_RUS, u"Скрыть"},
    {LANG_KAZ, u"Жасыру"},
    {LANG_BEL, u"Схаваць"},
    {LANG_UKR, u"Приховати"},
    {LANG_TUR, u"Gizle"},
};

static const TUtf16String& GetReadMoreText(ELanguage uiLang) {
    for (const auto& text : HIDE_TEXTS) {
        if (text.Lang == uiLang) {
            return text.ReadMoreText;
        }
    }
    return HIDE_TEXTS[0].ReadMoreText;
}

static float GetReservedLen(const TConfig& cfg) {
    const TUtf16String& readMoreText = GetReadMoreText(cfg.GetUILang());
    float pixelLength = GetStringPixelLength(readMoreText, cfg.GetSnipFontSize());
    return pixelLength / cfg.GetYandexWidth(); // len in pixels -> len in rows
}

float GetExtSnipLen(const TConfig& cfg, const TLengthChooser& lenCfg) {
    float res = 0;
    if (cfg.UseExtSnipRowLimit()) {
        res = cfg.GetExtSnipRowLimit() * lenCfg.GetRowLen() - GetReservedLen(cfg);
    } else {
        res = 3.0 * lenCfg.GetMaxSnipLen();
    }
    return res;
}

}
