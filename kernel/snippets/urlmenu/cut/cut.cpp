#include "cut.h"

#include <kernel/snippets/strhl/goodwrds.h>
#include <kernel/snippets/strhl/zonedstring.h>
#include <kernel/snippets/urlcut/urlcut.h>
#include <kernel/snippets/urlcut/consts.h>

#include <library/cpp/langs/langs.h>

#include <util/charset/wide.h>
#include <util/generic/utility.h>
#include <library/cpp/string_utils/url/url.h>

namespace NUrlMenu {
    static constexpr size_t DELIMITER_WIDTH = 3;
    static constexpr size_t MAX_TAIL_WIDTH = 25;
    static constexpr size_t MIN_TAIL_WIDTH = 10;

    static void TryCut(TUtf16String& s, size_t maxLen) {
        if (s.length() <= maxLen)
            return;
        if (maxLen <= NUrlCutter::Ellipsis.length())
            return;

        size_t pos = s.rfind(wchar16(' '), maxLen - NUrlCutter::Ellipsis.length());
        if (pos != TUtf16String::npos) {
            s = s.substr(0, pos);
            s.append(NUrlCutter::Ellipsis);
        }
    }

    THilitedUrlMenu::THilitedUrlMenu() {
    }

    THilitedUrlMenu::THilitedUrlMenu(const TUrlMenuVector& urlMenu) {
        for (const auto& item : urlMenu) {
            Items.emplace_back();
            Items.back().Url = WideToASCII(item.first);
            Items.back().Text = NUrlCutter::THilitedString(item.second);
        }
    }

    TUrlMenuVector THilitedUrlMenu::Merge(const TUtf16String& openMark, const TUtf16String& closeMark) const {
        TUrlMenuVector result;
        for (const auto& item : Items) {
            result.emplace_back(ASCIIToWide(item.Url), item.Text.Merge(openMark, closeMark));
        }
        return result;
    }

    TVector<TUtf16String> THilitedUrlMenu::GetHilitedWords() const {
        TVector<TUtf16String> hlWords;
        for (const auto& item : Items) {
            item.Text.FillHilitedWords(hlWords);
        }
        return hlWords;
    }

    void THilitedUrlMenu::SetHostTextFromUrl(const TString& url) {
        if (!Items) {
            return;
        }
        TStringBuf hostAndPortWithCase = GetHostAndPort(CutWWWPrefix(CutSchemePrefix(url)));
        Items[0].Text.String = ASCIIToWide(hostAndPortWithCase);
    }

    void THilitedUrlMenu::SetTailUrl(const TString& url) {
        if (!Items) {
            return;
        }
        Items.back().Url = url;
    }

    void THilitedUrlMenu::HiliteAndCut(size_t maxLength,
                                       NUrlCutter::TRichTreeWanderer& rTreeWanderer,
                                       const TInlineHighlighter& inlineHighlighter,
                                       ELanguage lang, const NSnippets::W2WTrie& domainTrie) {
        if (!Items) {
            return;
        }
        HiliteAndCutHost(maxLength, rTreeWanderer, lang, domainTrie);
        const bool hasTail = Items.back().Url.empty();
        if (hasTail) {
            CutRightPartsAndHiliteTail(maxLength, rTreeWanderer, lang);
        } else {
            CutLeftParts(maxLength);
        }
        HiliteMenuNames(inlineHighlighter);
    }

    void THilitedUrlMenu::HiliteAndCutHost(size_t maxLength,
                                           NUrlCutter::TRichTreeWanderer& rTreeWanderer,
                                           ELanguage lang, const NSnippets::W2WTrie& domainTrie) {
        if (!Items) {
            return;
        }

        const TString host = WideToASCII(Items[0].Text.String);
        Items[0].Text = NUrlCutter::HiliteAndCutUrlMenuHost(host, maxLength, rTreeWanderer, lang, domainTrie);
    }

    void THilitedUrlMenu::CutLeftParts(size_t maxLength) {
        if (!Items) {
            return;
        }

        size_t menuWidth = Items.front().Text.String.length();
        size_t firstMenuIndex = Items.size();
        if (menuWidth + Items.back().Text.String.length() + DELIMITER_WIDTH > maxLength &&
            maxLength > DELIMITER_WIDTH + menuWidth) {
            TryCut(Items.back().Text.String, maxLength - (DELIMITER_WIDTH + menuWidth));
        }
        for (size_t i = Items.size() - 1; i > 0; --i) {
            size_t nextWidth = menuWidth + Items[i].Text.String.length() + DELIMITER_WIDTH;
            if (nextWidth > maxLength) {
                break;
            }
            menuWidth = nextWidth;
            firstMenuIndex = i;
        }

        if (firstMenuIndex == Items.size()) {
            Items.clear();
        } else if (firstMenuIndex > 1) {
            Items.erase(Items.begin() + 1, Items.begin() + firstMenuIndex);
        }
    }

    void THilitedUrlMenu::CutRightPartsAndHiliteTail(size_t maxLength,
                                                     NUrlCutter::TRichTreeWanderer& rTreeWanderer,
                                                     ELanguage lang) {
        if (!Items) {
            return;
        }

        size_t menuWidth = 0;
        size_t menuSize = 0;
        size_t coveredUrlPart = 0;
        for (const auto& item : Items) {
            if (item.Url) {
                size_t nextWidth = menuWidth + item.Text.String.length() + DELIMITER_WIDTH;
                if (nextWidth + MIN_TAIL_WIDTH > maxLength) {
                    break;
                }
                ++menuSize;
                menuWidth = nextWidth;
                coveredUrlPart = item.Url.length();
            }
        }
        if (menuSize <= 1) {
            Items.clear();
            return;
        }

        TString tail;
        if (Items.size() >= menuSize + 2) {
            const auto& lastItemBeforeTail = Items[Items.size() - 2];
            tail = lastItemBeforeTail.Url.substr(coveredUrlPart);
            if (tail && tail.back() != '/') {
                tail += '/';
            }
        }
        tail += WideToASCII(Items.back().Text.String);
        while (tail && tail.back() == '/') {
            tail.pop_back();
        }

        Items.resize(menuSize);
        if (tail) {
            Items.emplace_back();
            const size_t tailMaxLen = Min(maxLength - menuWidth, MAX_TAIL_WIDTH);
            Items.back().Text = NUrlCutter::HiliteAndCutUrlMenuPath(tail, tailMaxLen, rTreeWanderer, lang);
        }
    }

    void THilitedUrlMenu::HiliteMenuNames(const TInlineHighlighter& inlineHighlighter) {
        for (size_t i = 1; i < Items.size(); ++i) {
            if (Items[i].Url) {
                TZonedString zoned = Items[i].Text.String;
                TPaintingOptions options;
                options.SrcOutput = true;
                inlineHighlighter.PaintPassages(zoned, options);
                for (const auto& entry : zoned.Zones) {
                    if (entry.second.Mark) {
                        for (const auto& span : entry.second.Spans) {
                            const size_t beginOffset = ~span - zoned.String.data();
                            const size_t endOffset = ~span + +span - zoned.String.data();
                            Items[i].Text.SortedHilitedSpans.push_back({beginOffset, endOffset});
                        }
                    }
                }
            }
        }
    }
}
