#include "generic_for_mobile.h"

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/qtree/query.h>
#include <kernel/snippets/replace/replace.h>
#include <kernel/snippets/sent_match/extra_attrs.h>
#include <kernel/snippets/sent_match/lang_check.h>
#include <kernel/snippets/sent_match/tsnip.h>

#include <kernel/snippets/strhl/goodwrds.h>
#include <kernel/snippets/strhl/zonedstring.h>
#include <kernel/snippets/strhl/glue_common.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_writer.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/charset/wide.h>

namespace NSnippets {

static const TUtf16String PASSAGE_SEPARATOR = u" ... ";

void TGenericForMobileDataReplacer::DoWork(TReplaceManager* manager) {
    if (!manager->IsBodyReplaced()) {
        return;
    }
    const TReplaceContext& repCtx = manager->GetContext();
    if (!repCtx.Cfg.IsTouchReport()) {
        return;
    }
    if (repCtx.IsByLink) {
        return;
    }
    const bool noCyr = repCtx.Cfg.SuppressCyrForTr() && !repCtx.QueryCtx.CyrillicQuery;
    const TSnip& snip = repCtx.Snip;
    TUtf16String descr;
    TVector<TZonedString> tmp = snip.GlueToZonedVec();
    for (TZonedString& it : tmp) {
        repCtx.IH.PaintPassages(it);
        if (descr) {
            descr += PASSAGE_SEPARATOR;
        }
        descr += MergedGlue(it);
    }
    if (noCyr && HasTooManyCyrillicWords(descr, 2)) {
        descr.clear();
    }
    if (descr) {
        NJson::TJsonValue features(NJson::JSON_MAP);
        features["genericsnip"] = NJson::TJsonValue(WideToUTF8(descr));
        NJson::TJsonValue serpInfo(NJson::JSON_MAP);
        serpInfo["format"] = "json";
        serpInfo["type"] = "mobile_extra";
        serpInfo["kind"] = "snippets";
        NJson::TJsonValue res(NJson::JSON_MAP);
        res["content_plugin"] = true;
        res["features"] = features;
        res["SerpInfo"] = serpInfo;
        manager->GetExtraSnipAttrs().AddClickLikeSnipJson("mobile_extra", NJson::WriteJson(res, false, true));
        manager->SetMarker(MRK_MOBILE_EXTRA);
    }
}

}

