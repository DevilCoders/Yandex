#pragma once

#include <util/generic/string.h>
#include <util/stream/output.h>

namespace NSnippets {

    enum EMetricName {
        MN_IS_EMPTY = 0                        /* "isEmpty" */,

        MN_TITLE_IS_URL                        /* "titleIsUrl" */,
        MN_TITLE_BOLDED_RATE                   /* "titleBoldedRate" */,
        MN_SNIPPET_BOLDED_RATE                 /* "snippetBoldedRate" */,
        MN_LINKS_RATE                          /* "linksRate" */,
        MN_LINKS_1_RATE                        /* "links1Rate" */,
        MN_LINKS_2_RATE                        /* "links2Rate" */,

        MN_SNIP_LEN_IN_SENTS                   /* "snipLenInSents" */,
        MN_SNIP_LEN_IN_WORDS                   /* "snipLenInWords" */,
        MN_SNIP_LEN_IN_BYTES                   /* "snipLenInChars" */,
        MN_TITLE_WORDS_IN_SNIP_PERCENT         /* "titleWordsInSnipPercent" */,
        MN_LESS_4_WORDS_SNIP_RATE              /* "less4WordsSnipRate" */,
        MN_HAS_SHORT_FRAGMENT_SNIP_RATE        /* "hasShortFragmentSnipRate" */,

        MN_MAX_CHAIN_LEN                       /* "maxChain" */,
        MN_HAS_QUERY_WORDS                     /* "hasQueryWords" */,
        MN_QUERY_WORDS_COUNT                   /* "queryWordsCount" */,
        MN_HAS_ALL_QUERY_WORDS                 /* "hasAllQueryWords" */,
        MN_QUERY_WORDS_FOUND_PERCENT           /* "queryWordsFoundPercent" */,
        MN_QUERY_USER_WORDS_FOUND_PERCENT      /* "queryUserWordsFoundPercent" */,
        MN_HAS_NO_QUERY_WORDS_IN_TITLE         /* "hasNoQueryWordsInTitle" */,
        MN_QUERY_WORDS_FOUND_IN_TITLE_PERCENT  /* "queryWordsFoundInTitlePercent" */,
        MN_MATCHES_COUNT                       /* "matchesCount" */,
        MN_QUERY_USER_WORDS_FOUND              /* "queryUserWordsFound" */,
        MN_QUERY_WORDS_FOUND                   /* "queryWordsFound" */,
        MN_NONSTOP_USER_EXTEN_COUNT            /* "nonstopUserExtenCount" */,
        MN_NONSTOP_USER_EXTEN_RATE             /* "nonstopUserExtenRate" */,
        MN_UNIQUE_WORD_RATE                    /* "uniqueWordRate" */,
        MN_UNIQUE_LEMMA_RATE                   /* "uniqueLemmaRate" */,
        MN_UNIQUE_GROUP_RATE                   /* "uniqueGroupRate" */,
        MN_NONSTOP_USER_WORD_DENSITY           /* "nonstopUserWordDensity" */,
        MN_NONSTOP_USER_LEMMA_DENSITY          /* "nonstopUserLemmaDensity" */,
        MN_NONSTOP_USER_EXTEN_DENSITY          /* "nonstopUserExtenDensity" */,
        MN_NONSTOP_USER_WIZ_COUNT              /* "nonstopUserWizCount" */,
        MN_NONSTOP_USER_WIZ_DENSITY            /* "nonstopUserWizDensity" */,
        MN_NONSTOP_USER_CLEAN_LEMMA_COUNT      /* "nonstopUserCleanLemmaCount" */,
        MN_NONSTOP_USER_CLEAN_LEMMA_DENSITY    /* "nonstopUserCleanLemmaDensity" */,
        MN_NONSTOP_USER_SYNONYMS_COUNT         /* "nonstopUserSynonymsCount" */,

        MN_FIRST_MATCH_POS                     /* "firstMatchPos" */,

        MN_HAS_CYRILLIC_CHARS                  /* "hasCyrillicChars" */,
        MN_NOT_READABLE_CHARS_RATE             /* "notReadableCharsRate" */,
        MN_DIFF_LANGUAGE_WORD_RATE             /* "diffLanguageWordRate" */,
        MN_UPPERCASE_LETTERS_RATE              /* "uppercaseLettersRate" */,
        MN_AVERAGE_WORD_LENGTH                 /* "averageWordLength" */,
        MN_FRAGMENTS_COUNT                     /* "fragmentsCount" */,
        MN_TRIPLE_FRAGMENTS_RATE               /* "tripleFragmentsRate" */,
        MN_DIGIT_WORDS_RATE                    /* "digitWordsRate" */,
        MN_TRASH_WORDS_RATE                    /* "trashWordsRate" */,
        MN_HAS_PORNO_WORDS                     /* "hasPornoWords" */,
        MN_HAS_MENU                            /* "hasMenu" */,
        MN_HAS_URL                             /* "hasUrl" */,
        MN_HAS_SITELINKS                       /* "hasSitelinks" */,
        MN_SITELINKS_COUNT                     /* "sitelinksCount" */,

        MN_YANDEX_SNIPPET_STRING_COUNT         /* "yandexSnippetStringCount" */,
        MN_YANDEX_SNIPPET_LAST_STRING_FILL     /* "yandexSnippetLastStringFill" */,
        MN_GOOGLE_SNIPPET_STRING_COUNT         /* "googleSnippetStringCount" */,
        MN_GOOGLE_SNIPPET_LAST_STRING_FILL     /* "googleSnippetLastStringFill" */,

        MN_SITELINKS_WORDS_COUNT               /* "sitelinksWordsCount" */,
        MN_SITELINKS_SYMBOLS_COUNT             /* "sitelinksSymbolsCount" */,
        MN_SITELINKS_COLORED_WORDS_RATE        /* "sitelinksColoredWordsRate" */,
        MN_SITELINKS_DIGITS_RATE               /* "sitelinksDigitsRate" */,
        MN_SITELINKS_UPPERCASE_LETTERS_RATE    /* "sitelinksUppercaseLettersRate" */,
        MN_SITELINKS_UNKNOWN_LANGUAGE          /* "sitelinksUnknownLanguage" */,
        MN_SITELINKS_DIFFERENT_LANGUAGE        /* "sitelinksDifferentLanguage" */,

        MN_HAS_BNA                             /* "hasBNA" */,

        MN_HAS_URLMENU                         /* "hasUrlmenu" */,
        MN_SHOWED_URLMENU                      /* "showedUrlmenu" */,

        MN_BY_LINK                             /* "bylink" */,

        MN_COUNT
    };

    class TMetricArray {
    private:
        struct TMetric {
            double Value;
            i32 Count;
        };
        TMetric Values[MN_COUNT];
    public:
        TMetricArray();
        void Reset();
        void SetValue(EMetricName name, double value);
        void SumValue(EMetricName name, double value);
        i32 Count(EMetricName name) const;
        double Value(EMetricName name) const;
        void WriteAsXml(const TString& description, IOutputStream* out) const;
    };
}

const TString& ToString(NSnippets::EMetricName metric);
