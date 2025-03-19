#pragma once

#include <kernel/snippets/replace/replace.h>

#include <kernel/tarc/docdescr/docdescr.h>

#include <library/cpp/langs/langs.h>
#include <util/generic/string.h>

namespace NSnippets
{
    bool IsYacaForced(const TReplaceContext& ctx);
    bool IsYacaBanned(const TReplaceContext& ctx);

    class TYacaData
    {
    public:
        TUtf16String Title;
        TUtf16String Desc;

        TYacaData(const TDocInfos& docInfos, const ELanguage lang, const TConfig& cfg);
    };

    class TYacaReplacer : public IReplacer {
    public:
        TYacaReplacer()
            : IReplacer("yaca")
        {
        }

        void DoWork(TReplaceManager* manager) override;
    };
}
