#pragma once

#include <kernel/snippets/custom/hostnames_data/hostnames.h>
#include <kernel/snippets/urlmenu/common/common.h>
#include <kernel/snippets/urlcut/hilited_string.h>

#include <library/cpp/langs/langs.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>

class TInlineHighlighter;
namespace NUrlCutter {
    class TRichTreeWanderer;
}

namespace NUrlMenu {
    class THilitedUrlMenu {
    public:
        class THilitedUrlMenuItem {
        public:
            TString Url;
            NUrlCutter::THilitedString Text;
        };
        TVector<THilitedUrlMenuItem> Items;

    public:
        THilitedUrlMenu();
        THilitedUrlMenu(const TUrlMenuVector& urlMenu);
        TUrlMenuVector Merge(const TUtf16String& openMark, const TUtf16String& closeMark) const;
        TVector<TUtf16String> GetHilitedWords() const;
        void SetHostTextFromUrl(const TString& url);
        void SetTailUrl(const TString& url);
        void HiliteAndCut(size_t maxLength, NUrlCutter::TRichTreeWanderer& rTreeWanderer, const TInlineHighlighter& inlineHighlighter, ELanguage lang = LANG_RUS, const NSnippets::W2WTrie& domainTrie = NSnippets::W2WTrie());

    private:
        void HiliteAndCutHost(size_t maxLength, NUrlCutter::TRichTreeWanderer& rTreeWanderer, ELanguage lang, const NSnippets::W2WTrie& domainTrie = NSnippets::W2WTrie());
        void CutLeftParts(size_t maxLength);
        void CutRightPartsAndHiliteTail(size_t maxLength, NUrlCutter::TRichTreeWanderer& rTreeWanderer, ELanguage lang);
        void HiliteMenuNames(const TInlineHighlighter& inlineHighlighter);
    };
}
