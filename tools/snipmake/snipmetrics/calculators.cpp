#include "calculators.h"

#include <tools/snipmake/common/nohtml.h>
#include <kernel/snippets/titles/url_titles.h>

#include <library/cpp/string_utils/url/url.h>
#include <util/string/strip.h>

namespace NSnippets {

    static bool TitleIsUrl(const TReqSnip& reqSnip) {
        //processing url
        TString url(WideToUTF8(reqSnip.Url));
        TString urlWithoutHttp(CutHttpPrefix(url, true));
        TUtf16String urlForTitle(ConvertUrlToTitle(urlWithoutHttp));

        //processing title
        TUtf16String title(NoHtml(StripString(reqSnip.TitleText)));
        size_t len = title.size();
        if (len >= 3 && TWtringBuf(title.data() + len - 3, 3) == u"...") {
            len -= 3;
        } else {
            if (len >= 1 && title[len - 1] == (TChar)0x2026) {
                --len; // one symbol ellipsis
            }
        }
        TWtringBuf titleWithoutEllipsis(title.data(), len);

        return len <= urlForTitle.size() && urlForTitle.substr(0, len) == titleWithoutEllipsis;
    }

    void TSnipMetricsCalculator::CalculateSerpMetrics(const TQueryy& /*query*/, const TReqSerp& reqSerp) {
        this->ProcessMetricValue(MN_HAS_BNA, reqSerp.HasBNA);
    }

    void TSnipMetricsCalculator::CalculateMetrics(const TQueryy& query, const TReqSnip& reqSnip) {
        this->ProcessMetricValue(MN_TITLE_IS_URL, TitleIsUrl(reqSnip));


        this->ProcessMetricValue(MN_HAS_SITELINKS, reqSnip.Sitelinks.size() != 0);
        if (reqSnip.Sitelinks.size()) {
            this->ProcessMetricValue(MN_SITELINKS_COUNT, reqSnip.Sitelinks.size());
        }

        for (const TSitelink& sitelink : reqSnip.Sitelinks) {
            const TSitelinkStatInfo slInfo(sitelink, query, *Cfg);
            this->ProcessMetricValue(MN_SITELINKS_WORDS_COUNT, slInfo.WordsCount);
            this->ProcessMetricValue(MN_SITELINKS_SYMBOLS_COUNT, slInfo.SymbolsCount);
            this->ProcessMetricValue(MN_SITELINKS_COLORED_WORDS_RATE, slInfo.ColoredWordsRate);
            this->ProcessMetricValue(MN_SITELINKS_DIGITS_RATE, slInfo.DigitsRate);
            this->ProcessMetricValue(MN_SITELINKS_UPPERCASE_LETTERS_RATE, slInfo.UppercaseLettersRate);
            if (slInfo.HasAlpha) {
                this->ProcessMetricValue(MN_SITELINKS_UNKNOWN_LANGUAGE, slInfo.IsUnknownLanguage);
                this->ProcessMetricValue(MN_SITELINKS_DIFFERENT_LANGUAGE, slInfo.IsDifferentLanguage);
            }
        }

        if (reqSnip.SnipText.empty()) {
            this->ProcessMetricValue(MN_IS_EMPTY, 1);
            return;
        }
        const TSnipExtInfo snipExtInfo(query, reqSnip, *Cfg);
        this->ProcessMetricValue(MN_IS_EMPTY, 0);
        this->ProcessMetricValue(MN_BY_LINK, snipExtInfo.IsByLink);

        const TColoringInfo coloring(snipExtInfo, reqSnip);
        this->ProcessMetricValue(MN_TITLE_BOLDED_RATE, coloring.TitleBoldedRate);
        this->ProcessMetricValue(MN_SNIPPET_BOLDED_RATE, coloring.SnippetBoldedRate);
        this->ProcessMetricValue(MN_LINKS_RATE, coloring.LinksCount > 0);
        this->ProcessMetricValue(MN_LINKS_1_RATE, coloring.LinksCount == 1);
        this->ProcessMetricValue(MN_LINKS_2_RATE, coloring.LinksCount == 2);

        const TSnipStatInfo snInfo(snipExtInfo);
        this->ProcessMetricValue(MN_SNIP_LEN_IN_SENTS, snInfo.SnipLenInSents);
        this->ProcessMetricValue(MN_SNIP_LEN_IN_WORDS, snInfo.SnipLenInWords);
        this->ProcessMetricValue(MN_SNIP_LEN_IN_BYTES, snInfo.SnipLenInBytes);
        this->ProcessMetricValue(MN_TITLE_WORDS_IN_SNIP_PERCENT, snInfo.TitleWordsInSnipPercent);
        this->ProcessMetricValue(MN_LESS_4_WORDS_SNIP_RATE, snInfo.SnipLenInWords < 4);
        this->ProcessMetricValue(MN_HAS_SHORT_FRAGMENT_SNIP_RATE, snInfo.Less4WordsFragmentsCount);

        const TQueryWordsInSnipInfo qwInfo(snipExtInfo);
        this->ProcessMetricValue(MN_MAX_CHAIN_LEN, qwInfo.MaxChainLen);
        this->ProcessMetricValue(MN_HAS_QUERY_WORDS, qwInfo.NonstopLikeUserLemmaCount > 0);
        this->ProcessMetricValue(MN_QUERY_WORDS_COUNT, qwInfo.QueryWordsCount);
        this->ProcessMetricValue(MN_HAS_ALL_QUERY_WORDS, qwInfo.NonstopLikeUserLemmaCount >= qwInfo.NonstopQueryWordsCount);
        this->ProcessMetricValue(MN_QUERY_WORDS_FOUND_PERCENT, qwInfo.NonstopUserLemmaRate);
        this->ProcessMetricValue(MN_QUERY_USER_WORDS_FOUND_PERCENT, qwInfo.NonstopUserWordRate);
        this->ProcessMetricValue(MN_HAS_NO_QUERY_WORDS_IN_TITLE, qwInfo.QueryWordsFoundInTitlePercent == 0.0);
        this->ProcessMetricValue(MN_QUERY_WORDS_FOUND_IN_TITLE_PERCENT, qwInfo.QueryWordsFoundInTitlePercent);
        this->ProcessMetricValue(MN_MATCHES_COUNT, qwInfo.MatchesCount);                            // всего найдено без уникальности
        this->ProcessMetricValue(MN_QUERY_USER_WORDS_FOUND, qwInfo.NonstopUserWordCount);           // точные
        this->ProcessMetricValue(MN_QUERY_WORDS_FOUND, qwInfo.NonstopLikeUserLemmaCount);           // + леммы
        this->ProcessMetricValue(MN_NONSTOP_USER_EXTEN_COUNT, qwInfo.NonstopUserExtenCount);        // + расширения
        this->ProcessMetricValue(MN_NONSTOP_USER_EXTEN_RATE, qwInfo.NonstopUserExtenRate);
        this->ProcessMetricValue(MN_UNIQUE_WORD_RATE, qwInfo.UniqueWordRate);
        this->ProcessMetricValue(MN_UNIQUE_LEMMA_RATE, qwInfo.UniqueLemmaRate);
        this->ProcessMetricValue(MN_UNIQUE_GROUP_RATE, qwInfo.UniqueGroupRate);
        this->ProcessMetricValue(MN_NONSTOP_USER_WORD_DENSITY, qwInfo.NonstopUserWordDensity);
        this->ProcessMetricValue(MN_NONSTOP_USER_LEMMA_DENSITY, qwInfo.NonstopUserLemmaDensity);
        this->ProcessMetricValue(MN_NONSTOP_USER_EXTEN_DENSITY, qwInfo.NonstopUserExtenDensity);
        this->ProcessMetricValue(MN_NONSTOP_USER_WIZ_COUNT, qwInfo.NonstopUserWizCount);
        this->ProcessMetricValue(MN_NONSTOP_USER_WIZ_DENSITY, qwInfo.NonstopUserWizDensity);
        this->ProcessMetricValue(MN_NONSTOP_USER_CLEAN_LEMMA_COUNT, qwInfo.NonstopCleanUserLemmaCount);
        this->ProcessMetricValue(MN_NONSTOP_USER_CLEAN_LEMMA_DENSITY, qwInfo.NonstopUserCleanLemmaDensity);
        this->ProcessMetricValue(MN_NONSTOP_USER_SYNONYMS_COUNT, qwInfo.NonstopUserSynonymsCount);

        if (qwInfo.FirstMatchPos >= 0)
            this->ProcessMetricValue(MN_FIRST_MATCH_POS, qwInfo.FirstMatchPos);

        TSymbolsStat charsStat;
        charsStat.CodePage("Cyrillic").add(0x0410, 0x044F); // main russian alphabet
        const TSnipReadabilityInfo readInfo(snipExtInfo, this->PornoWeighter, charsStat);
        this->ProcessMetricValue(MN_HAS_CYRILLIC_CHARS, charsStat.IsMatched("Cyrillic"));
        this->ProcessMetricValue(MN_NOT_READABLE_CHARS_RATE, readInfo.NotReadableCharsRate);
        this->ProcessMetricValue(MN_DIFF_LANGUAGE_WORD_RATE, readInfo.DiffLanguageWordsRate);
        this->ProcessMetricValue(MN_UPPERCASE_LETTERS_RATE, readInfo.UppercaseLettersRate);
        this->ProcessMetricValue(MN_AVERAGE_WORD_LENGTH, readInfo.AverageWordLength);
        this->ProcessMetricValue(MN_FRAGMENTS_COUNT, readInfo.FragmentsCount);
        this->ProcessMetricValue(MN_TRIPLE_FRAGMENTS_RATE, readInfo.FragmentsCount > 2);
        this->ProcessMetricValue(MN_DIGIT_WORDS_RATE, readInfo.DigitsRate);
        this->ProcessMetricValue(MN_TRASH_WORDS_RATE, readInfo.TrashWordsRate);
        this->ProcessMetricValue(MN_HAS_PORNO_WORDS, readInfo.HasPornoWords);
        this->ProcessMetricValue(MN_HAS_MENU, readInfo.HasMenuLike);
        this->ProcessMetricValue(MN_HAS_URL, readInfo.HasUrl);

        const TPixelLengthInfo pixelLengthInfo(snipExtInfo);
        this->ProcessMetricValue(MN_YANDEX_SNIPPET_STRING_COUNT, pixelLengthInfo.GetYandexStringNum());
        this->ProcessMetricValue(MN_YANDEX_SNIPPET_LAST_STRING_FILL, pixelLengthInfo.GetYandexLastStringFill());
        this->ProcessMetricValue(MN_GOOGLE_SNIPPET_STRING_COUNT, pixelLengthInfo.GetGoogleStringNum());
        this->ProcessMetricValue(MN_GOOGLE_SNIPPET_LAST_STRING_FILL, pixelLengthInfo.GetGoogleLastStringFill());
    }
}
