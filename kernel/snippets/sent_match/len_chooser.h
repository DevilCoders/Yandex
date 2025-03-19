#pragma once

#include <kernel/snippets/idl/enums.h>

namespace NSnippets {
    class TConfig;
    class TQueryy;

    class TLengthChooser {
    private:
        const TConfig& Cfg;
        const TQueryy& Query;
        const ETextCutMethod CutMethod;
        bool ShortenRowScale = false;

    public:
        TLengthChooser(const TConfig& cfg, const TQueryy& query);

        // SNIPPETS-1139
        float GetMaxSnipLen(int frags = 0) const;
        float GetRowLen() const;
        float GetMaxSpecSnipLen() const;
        float GetMaxTextSpecSnipLen() const;
        float GetNDesktopRowsLen(int rows) const;
        float GetMaxByLinkSnipLen() const;

        void SetShortenRowScale();

    private:
        float SymbolsToWholeRows(int symbols) const;
    };

}
