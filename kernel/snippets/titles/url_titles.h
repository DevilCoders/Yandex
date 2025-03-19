#pragma once

#include <library/cpp/langs/langs.h>
#include <util/generic/hash_set.h>
#include <util/generic/string.h>

class TWordFilter;
namespace NLemmer {
    class TAlphabetWordNormalizer;
}

namespace NSnippets {

class TQueryy;
class TSentsInfo;

class TUrlTitleTokenizer {
private:
    const TQueryy& Query;
    const TSentsInfo* SentsInfo;
    THashSet<TUtf16String> Words;
    const NLemmer::TAlphabetWordNormalizer* WordNormalizer;
    const TWordFilter& StopWords;

    void InitWords();
    bool TryToSplitByDictionary(TUtf16String& token);
    bool TryToSplitBySeparators(TUtf16String& token) const;
    bool TryToSplit(TUtf16String& token);
    bool TokenLooksNatural(const TUtf16String& token);

public:
    TUrlTitleTokenizer(const TQueryy& query, const TSentsInfo* sentsInfo, ELanguage lang, const TWordFilter& stopWords);
    bool GenerateUrlBasedTitle(TUtf16String& wurl, const TUtf16String& titleString = TUtf16String());
};

    TUtf16String ConvertUrlToTitle(const TString& url);
}
