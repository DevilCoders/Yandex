#include "clicklike.h"
#include <kernel/snippets/replace/replace.h>
#include <kernel/snippets/sent_match/extra_attrs.h>
#include <library/cpp/json/json_reader.h>
#include <util/generic/strbuf.h>

namespace NSnippets {

namespace NAttrs {
    const TStringBuf WILDCARD_PREFIX = "clicklike_";
    const TStringBuf LEGACY_RECIPE = "foto_recipe";
    EMarker LEGACY_RECIPE_MARKER = MRK_FOTO_RECIPE;
}

void TClickLikeSnipDataReplacer::DoWork(TReplaceManager* manager) {
    Y_ASSERT(nullptr != manager);

    const TReplaceContext& repCtx = manager->GetContext();
    for (auto&& attr : repCtx.DocInfos) {
        TStringBuf attrName(attr.first);
        TStringBuf attrValue(attr.second);
        bool useAttr = false;
        EMarker marker = MRK_COUNT;

        if (attrName.StartsWith(NAttrs::WILDCARD_PREFIX)) {
            useAttr = true;
            attrName = attrName.SubStr(NAttrs::WILDCARD_PREFIX.size());
        }
        else if (attrName == NAttrs::LEGACY_RECIPE) {
            useAttr = true;
            marker = NAttrs::LEGACY_RECIPE_MARKER;
        }

        if (!useAttr || !attrValue || !attrName) {
            continue;
        }
        if (!NJson::ValidateJson(attrValue)) {
            continue;
        }
        manager->GetExtraSnipAttrs().AddClickLikeSnipJson(TString(attrName), TString(attrValue));
        if (marker < MRK_COUNT) {
            manager->SetMarker(marker);
        }
    }
}

}
