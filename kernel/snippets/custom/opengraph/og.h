#pragma once

#include <kernel/snippets/replace/replace.h>

namespace NSnippets {

    TUtf16String GetOgDescr(const TDocInfos& infos);

    struct TOgTitleData {
        TUtf16String Title = TUtf16String();
        TUtf16String Sitename = TUtf16String();
        bool HasAttrs = false;
        TOgTitleData(const TDocInfos& docInfos);
    };

    class TOgTextReplacer : public IReplacer {
    public:
        TOgTextReplacer();
        void DoWork(TReplaceManager* manager) override;
    };

}
