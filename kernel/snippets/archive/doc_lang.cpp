#include "doc_lang.h"

#include <kernel/snippets/config/config.h>

#include <kernel/lemmer/core/language.h>

#include <library/cpp/langs/langs.h>

namespace NSnippets {
    ELanguage GetDocLanguage(const TDocInfos& docInfos, const TConfig& cfg) {
        ELanguage docLang = LANG_UNK;
        TDocInfos::const_iterator langAttr = docInfos.find("lang");
        if (langAttr != docInfos.end()) {
            docLang = LanguageByName(langAttr->second);
        } else {
            docLang = cfg.GetForeignNavHackLang();
        }
        if (!NLemmer::GetLanguageById(docLang)) {
            return LANG_UNK; // languages is not supported by lemmer
        }
        return docLang;
    }
}
