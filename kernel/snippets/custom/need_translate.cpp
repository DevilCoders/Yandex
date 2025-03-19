#include "need_translate.h"

#include <kernel/snippets/archive/doc_lang.h>
#include <kernel/snippets/config/config.h>
#include <kernel/snippets/sent_match/extra_attrs.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/langs/langs.h>

namespace NSnippets {

TNeedTranslateReplacer::TNeedTranslateReplacer()
    : IReplacer("need_translate")
{
}

void TNeedTranslateReplacer::DoWork(TReplaceManager* manager) {
    const TReplaceContext& ctx = manager->GetContext();

    ELanguage docLang = GetDocLanguage(ctx.DocInfos, ctx.Cfg);
    if (docLang == LANG_UNK){
        return;
    }

    ELanguage uiLang = ctx.Cfg.GetUILang();
    if (uiLang == docLang){
        return;
    }

    NJson::TJsonValue features(NJson::JSON_MAP);
    features["type"] = "need_translate";
    features["user_lang"] = NameByLanguage(uiLang);
    features["doc_lang"] = NameByLanguage(docLang);

    NJson::TJsonValue data(NJson::JSON_MAP);
    data["block_type"] = "construct";
    data["content_plugin"] = true;
    data["features"] = features;
    manager->GetExtraSnipAttrs().AddClickLikeSnipJson("need_translate", NJson::WriteJson(data));
}

} // namespace NSnippets
