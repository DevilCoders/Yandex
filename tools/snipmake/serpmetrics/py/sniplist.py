#!/usr/bin/python
# -*- coding: utf-8 -*-

metrics_base = [
        ("matchesCount"                     , "Число найденных слов запроса"),                                                      # 9
        ("queryUserWordsFound"              , "Число точных слов запроса"),                                                         # 4
        ("queryWordsFound"                  , "Число точных слов запроса + леммы"),                                                 # 6
        ("nonstopUserExtenCount"            , "Число точных слов запроса + леммы + расширения"),                                    # 9
        ("nonstopUserCleanLemmaCount"       , "Число лемм"),
        ("nonstopUserWizCount"              , "Число расширений"),
        ("nonstopUserSynonymsCount"         , "Число синонимов"),
        ("queryUserWordsFoundPercent"       , "Доля точных слов запроса"),                                                          # 3
        ("queryWordsFoundPercent"           , "Доля точных слов запроса + леммы"),                                                  # 5
        ("nonstopUserExtenRate"             , "Доля точных слов запроса + леммы + расширения"),                                     # 9
        ("nonstopUserCleanLemmaDensity"     , "Доля лемм"),
        ("nonstopUserWizDensity"            , "Доля расширений"),
        ("hasAllQueryWords"                 , "Доля сниппетов, содержащих все слова запроса"),                                      # 45
        ("hasQueryWords"                    , "Доля сниппетов, содержащих слова запроса"),                                          # 8
        ("queryWordsFoundInTitlePercent"    , "Доля слов запроса, содержащихся в заголовке документа"),                             # 51
        ("titleWordsInSnipPercent"          , "Доля слов заголовка, встречающихся в сниппете"),                                     # 19
        ("firstMatchPos"                    , "Позиция первого вхождения слова запроса"),                                           # 10
        ("maxChain"                         , "Длина максимальной цепочки вхождений слов запроса"),                                 # 1
        ("snipLenInWords"                   , "Длина сниппета в словах"),                                                           # 15
        ("snipLenInChars"                   , "Длина снипета в символах"),                                                          # 16
        ("snipLenInSents"                   , "Длина сниппета в предложениях"),                                                     # 14
        ("hasUrl"                           , "Доля сниппетов, содержащих URL в сниппете"),                                         # 25
        ("hasMenu"                          , "Идущие подряд три слова с заглавной буквы"),                                         # 26
        ("hasSitelinks"                     , "Доля документов, содержащих сайтлинки"),
        ("sitelinksCount"                   , "Число сайтлинков в сниппете"),
        ("sitelinksWordsCount"              , "Число слов в заголовке сайтлинка"),
        ("sitelinksSymbolsCount"            , "Число символов в заголовке сайтлинка"),
        ("sitelinksColoredWordsRate"        , "Плотность прокраски слов заголовка сайтлинков"),
        ("sitelinksDigitsRate"              , "Плотность цифр в заголовке сайтлинка"),
        ("sitelinksUppercaseLettersRate"    , "Плотность заглавных букв в заголовке сайтлинка"),
        ("sitelinksUnknownLanguage"         , "Доля сайтлинков с нераспознанным языком"),
        ("sitelinksDifferentLanguage"       , "Доля сайтлинков на языке, отличном от языка запроса"),
        ("bnoLinksEmptyAnnotations"         , "Доля ссылок в БНО с пустыми аннотациями"),
        ("hasBNA"                           , "Доля запросов с БНО"),
        ("diffLanguageWordRate"             , "Доля слов на другом языке"),                                                         # 24
        ("trashWordsRate"                   , "Доля мусорных слов в сниппете"),                                                     # 27
        ("notReadableCharsRate"             , "Доля нечитабельных символов в сниппете"),                                            # 23
        ("queryWordsCount"                  , "Кол-во слов в запросе"),
        ("averageWordLength"                , "Средняя длина слов"),                                                                # 28
        ("fragmentsCount"                   , "Число фрагментов в сниппете"),                                                       # 30
        ("tripleFragmentsRate"              , "Число трех+ фрагментных сниппетов"),
        ("isEmpty"                          , "Доля пустых сниппетов"),                                                             # 44
        ("bylink"                           , "Доля НПС сниппетов с пустым headline"),
        ("less4WordsSnipRate"               , "Доля сниппетов короче четырех слов"),
        ("hasShortFragmentSnipRate"         , "Доля сниппетов с фрагментами короче четырех слов"),
        ("digitWordsRate"                   , "Доля цифр"),
        ("uniqueWordRate"                   , "Доля уникальных слов"),
        ("uniqueLemmaRate"                  , "Доля уникальных лемм"),
        ("uniqueGroupRate"                  , "Доля уникальных групп слов"),
        ("nonstopUserWordDensity"           , "Плотность прокраски точных слов запроса"),
        ("nonstopUserLemmaDensity"          , "Плотность прокраски точных слов запроса + леммы"),
        ("nonstopUserExtenDensity"          , "Плотность прокраски точных слов запроса + леммы + расширения"),
        ("titleBoldedRate"                  , "Плотность прокраски заголовка по bold"),
        ("snippetBoldedRate"                , "Плотность прокраски сниппета по bold"),
        ("uppercaseLettersRate"             , "Доля символов в верхнем регистре"),
        ("hasPornoWords"                    , "Доля сниппетов, содержащих порно слова"),
        ("hasCyrillicChars"                 , "Доля сниппетов, содержащих русские символы"),                                        # 53
        ("hasNoQueryWordsInTitle"           , "Доля сниппетов, в которых заголовок не содержит слов запроса"),                      # 52
        ("yandexSnippetStringCount"         , "Число строк в сниппете в серпе Яндекса"),                                            # 47
        ("googleSnippetStringCount"         , "Число строк в сниппете в серпе Google"),                                             # 49
        ("yandexSnippetLastStringFill"      , "Заполненность последней строки сниппета в серпе Яндекса"),                           # 48
        ("googleSnippetLastStringFill"      , "Заполненность последней строки сниппета в серпе Google"),                            # 50
        ("titleIsUrl"                       , "Доля заголовков из урлов"),
        ("hasUrlmenu"                       , "Доля документов с данными для UrlMenu"),
        ("showedUrlmenu"                    , "Доля документов с хлебными крошками из gta _UrlMenu"),
        ("favIconsRate"                     , "Доля документов с favicon")
]

razladki_base = dict([
    ("snippetBoldedRate", "snipbold_rate_avg"),
    ("notReadableCharsRate", "notreadable_chars_rate_avg"),
    ("uppercaseLettersRate", "uppercase_letters_rate_avg"),
    ("digitWordsRate", "digit_words_rate_avg"),
    ("diffLanguageWordRate", "diff_language_word_rate_avg"),
    ("snipLenInChars", "symbol_count_avg"),
    ("isEmpty", "emptysnip_doc_coverage"),
    ("titleIsUrl", "title_is_url_doc_coverage"),
    ("hasPornoWords", "with_pornowords_doc_coverage"),
    ("hasCyrillicChars", "with_cyrillic_doc_coverage"),
    ("hasBNA", "bna_req_coverage"),
    ("hasSitelinks", "sitelinks_doc_coverage"),
    ("less4WordsSnipRate", "less4words_snip_doc_coverage"),
    ("titleBoldedRate", "titlebold_rate_avg"),
    ("favIconsRate", "favicon_doc_coverage"),
    ("bylink", "bylink_only_doc_coverage")
])
