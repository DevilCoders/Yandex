#include "meta.h"
#include "lang_check.h"

#include <kernel/snippets/config/config.h>

#include <util/charset/wide.h>
#include <util/generic/strbuf.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/string/subst.h>

namespace NSnippets
{
    TMetaDescription::TMetaDescription(const TConfig& cfg, const TDocInfos& docInfos, const TString& url, ELanguage docLang, bool isNav)
    {
        TDocInfos::const_iterator metaDescr = docInfos.find("ultimate_description");
        if (metaDescr == docInfos.end()) {
            return;
        }
        bool lowQuality = false;
        TDocInfos::const_iterator metaQuality = docInfos.find("meta_quality");
        if (metaQuality == docInfos.end() || TStringBuf(metaQuality->second) != "yes") {
            lowQuality = true;
        }
        TUtf16String text = UTF8ToWide(metaDescr->second);
        Collapse(text);
        Strip(text);
        if (!text) {
            return;
        }

        if (CutWWWPrefix(GetOnlyHost(url)) == TStringBuf("instagram.com")) {
            SubstGlobal(text, u"&#39;", u"'");
        }

        if (lowQuality) {
            LowQualityText = text;
        } else {
            ELanguage foreignNavHackLang = LANG_UNK;
            if (cfg.NavYComLangHack() && (isNav || cfg.IsMainPage())) {
                foreignNavHackLang = cfg.GetForeignNavHackLang();
            }
            ELanguage checkLang = foreignNavHackLang;
            if (checkLang == LANG_UNK) {
                checkLang = docLang;
            }
            Good = checkLang == LANG_UNK || HasWordsOfAlphabet(text, checkLang);
            Text = text;
        }
    }

    TUtf16String TMetaDescription::GetTextCopy() const
    {
        return Text;
    }
    const TUtf16String& TMetaDescription::GetLowQualityText() const
    {
        return LowQualityText;
    }
    bool TMetaDescription::MayUse() const
    {
        return Good;
    }
}
