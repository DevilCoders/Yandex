#include "translated_doc.h"

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/sent_match/extra_attrs.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/langs/langs.h>
#include <kernel/snippets/strhl/goodwrds.h>

namespace NSnippets {

    TTranslatedDocReplacer::TTranslatedDocReplacer()
            : IReplacer("translated_doc")
    {
    }

    void TTranslatedDocReplacer::DoWork(TReplaceManager* manager) {
        AddTranslatedDocSnippet(manager);
        AddTranslatedOriginalDocSnippet(manager);
    }

    void TTranslatedDocReplacer::AddTranslatedDocSnippet(TReplaceManager* manager) const {
        const TReplaceContext& ctx = manager->GetContext();
        auto urlIt = ctx.DocInfos.find("foreign_url");
        auto titleIt = ctx.DocInfos.find("foreign_title");
        auto langIt = ctx.DocInfos.find("foreign_lang");
        if (urlIt == ctx.DocInfos.end() || titleIt == ctx.DocInfos.end() || langIt == ctx.DocInfos.end()) {
            return;
        }

        NJson::TJsonValue features(NJson::JSON_MAP);
        features["type"] = "translated_doc";
        features["foreign_url"] = urlIt->second;

        auto title = UTF8ToWide(titleIt->second);
        ctx.IH.PaintPassages(title, TPaintingOptions::DefaultSnippetOptions());

        features["foreign_title"] = WideToUTF8(title);
        features["foreign_lang"] = langIt->second;

        NJson::TJsonValue data(NJson::JSON_MAP);
        data["block_type"] = "construct";
        data["content_plugin"] = true;
        data["features"] = features;
        data["disabled"] = false;
        manager->GetExtraSnipAttrs().AddClickLikeSnipJson("translated_doc", NJson::WriteJson(data));
    }

    void TTranslatedDocReplacer::AddTranslatedOriginalDocSnippet(TReplaceManager* manager) const {
        const TReplaceContext& ctx = manager->GetContext();
        auto urlIt = ctx.DocInfos.find("translated_url");
        if (urlIt == ctx.DocInfos.end()) {
            return;
        }

        NJson::TJsonValue features(NJson::JSON_MAP);
        features["type"] = "translated_original_doc";
        features["translated_url"] = urlIt->second;

        NJson::TJsonValue data(NJson::JSON_MAP);
        data["block_type"] = "construct";
        data["content_plugin"] = true;
        data["features"] = features;
        data["disabled"] = false;
        manager->GetExtraSnipAttrs().AddClickLikeSnipJson("translated_original_doc", NJson::WriteJson(data));
    }

} // namespace NSnippets
