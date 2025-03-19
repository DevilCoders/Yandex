#pragma once

#include <kernel/snippets/replace/replace.h>

#include <kernel/tarc/docdescr/docdescr.h>

#include <library/cpp/langs/langs.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>

namespace NSnippets
{
    class TConfig;

    bool DmozBanned(const TConfig& cfg, const TDocInfos& docInfos);

    class TDmozData {
        class TImpl;
        THolder<TImpl> Impl;
    public:
        explicit TDmozData(const TDocInfos& docInfos);
        ~TDmozData();
        bool FindByLanguage(ELanguage lang, TUtf16String& title, TUtf16String& desc);
    };

    class TDmozReplacer : public IReplacer {
    public:
        TDmozReplacer()
            : IReplacer("dmoz")
        {
        }

        void DoWork(TReplaceManager* manager) override;
    };
}
