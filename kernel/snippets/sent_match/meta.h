#pragma once

#include <kernel/tarc/docdescr/docdescr.h>

#include <library/cpp/langs/langs.h>
#include <util/generic/string.h>

namespace NSnippets
{
    class TConfig;

    class TMetaDescription
    {
    private:
        TUtf16String LowQualityText;
        TUtf16String Text;
        bool Good = false;

    public:
        TMetaDescription(const TConfig& cfg, const TDocInfos& docInfos, const TString& url, ELanguage docLang, bool isNav);
        TUtf16String GetTextCopy() const;
        const TUtf16String& GetLowQualityText() const;
        bool MayUse() const;
    };
}
