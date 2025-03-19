#pragma once

#include "hilited_string.h"
#include <kernel/snippets/custom/hostnames_data/hostnames.h>
#include <library/cpp/langs/langs.h>
#include <util/generic/string.h>
#include <library/cpp/charset/doccodes.h>

namespace NUrlCutter {
    class TRichTreeWanderer;

    THilitedString HiliteAndCutUrl(
        const TString& url,
        size_t maxLen,
        size_t thresholdLen,
        TRichTreeWanderer& rTreeWanderer,
        ELanguage lang = LANG_RUS,
        ECharset docEncoding = CODES_UNKNOWN,
        bool cutPrefix = true,
        const NSnippets::W2WTrie& domainTrie = NSnippets::W2WTrie());

    THilitedString HiliteAndCutUrlMenuHost(
        const TString& host,
        size_t maxLen,
        TRichTreeWanderer& rTreeWanderer,
        ELanguage lang,
        const NSnippets::W2WTrie& domainTrie = NSnippets::W2WTrie());

    THilitedString HiliteAndCutUrlMenuPath(
        const TString& urlPath,
        size_t maxLen,
        TRichTreeWanderer& rTreeWanderer,
        ELanguage lang);
}
