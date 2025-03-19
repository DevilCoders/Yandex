#include "url_titles.h"

#include <kernel/snippets/titles/make_title/util_title.h>

#include <kernel/snippets/qtree/query.h>
#include <kernel/snippets/sent_info/sent_info.h>

#include <kernel/snippets/urlcut/decode_url.h>
#include <kernel/snippets/urlcut/preprocess.h>

#include <kernel/lemmer/alpha/abc.h>
#include <library/cpp/stopwords/stopwords.h>

#include <util/charset/wide.h>
#include <util/generic/vector.h>
#include <library/cpp/string_utils/url/url.h>

namespace NSnippets {

static const TUtf16String TURKEY_TOKEN(u"turkiye");
static const TUtf16String ONLINE_TOKEN(u"online");
static const TUtf16String WWW_PREFIX(u"www");
static constexpr size_t MINIMAL_TOKEN_LENGTH = 4;
static constexpr size_t MAXIMAL_TOKEN_LENGTH = 12;
static constexpr size_t MINIMAL_SPLIT_LENGTH = 7;
static constexpr size_t MAXIMAL_SPLIT_LENGTH = 30;
static constexpr size_t MAXIMAL_TOKENS_NUMBER = 6;
static constexpr double MAXIMAL_TOKENS_RATIO = 0.4;
static constexpr double LOW_CAPITALS_RATIO = 0.2;
static constexpr double MIN_HOSTNAME_SIMILARITY_RATIO = 0.6;

void TUrlTitleTokenizer::InitWords() {
    if (Words.empty() && SentsInfo != nullptr) {
        for (int i = 0; i < SentsInfo->WordCount(); ++i) {
            TUtf16String word(SentsInfo->GetWordBuf(i));
            word.to_lower();
            Words.insert(word);
        }
    }
}

//Splitting token using words mostly from among the query forms and the snippet text ones
bool TUrlTitleTokenizer::TryToSplitByDictionary(TUtf16String& token) {
    if (token.size() < MINIMAL_SPLIT_LENGTH || token.size() > MAXIMAL_SPLIT_LENGTH)
        return false;
    InitWords();
    int n = token.size();
    TVector<int> splitDP(n, -1);
    for (int i = n - 1; i >= 0; --i) {
        TUtf16String word = token.substr(i);
        for (int j = n - 1; j >= i; --j) {
            if (Query.LowerForms.contains(word) || Words.contains(word) || word == TURKEY_TOKEN || word == ONLINE_TOKEN) {
                if (j + 1 == n || splitDP[j + 1] != -1) {
                    splitDP[i] = j + 1;
                    break;
                }
            }
            word.pop_back();
        }
    }
    if (splitDP[0] == -1 || splitDP[0] == n)
         return false;
    int pos = 0;
    TUtf16String splittedToken;
    size_t tokensNumber = 0;
    while (pos != n) {
        TUtf16String word = token.substr(pos, splitDP[pos] - pos);
        if (!word.empty() && !word.StartsWith(WWW_PREFIX) && word.size() >= 2 && !StopWords.IsStopWord(word))
            word[0] = WordNormalizer->ToUpper(word[0]);
        if (pos != 0)
            splittedToken.push_back(' ');
        splittedToken += word;
        ++tokensNumber;
        pos = splitDP[pos];
    }
    if (tokensNumber > MAXIMAL_TOKENS_NUMBER || tokensNumber > MAXIMAL_TOKENS_RATIO * n)
        return false;
    token = splittedToken;
    return true;
}

//Splitting token by natural separators: capitals, hyphens, underscores
bool TUrlTitleTokenizer::TryToSplitBySeparators(TUtf16String& token) const {
    bool splitByHyphen = (token.find('-') != token.rfind('-'));
    TUtf16String upperToken(token);
    WordNormalizer->ToUpper(upperToken);
    bool splitUpper = (token != upperToken);
    if (token.find('-') != TUtf16String::npos || token.find('_') != TUtf16String::npos)
        splitUpper = false;
    size_t n = token.size();
    size_t tokensNumber = 0;
    size_t j = 0;
    TUtf16String splittedToken;
    for (size_t i = 0; i <= n; ++i) {
        if (i == n || token[i] == '_' || (splitByHyphen && token[i] == '-') || (i > 0 && splitUpper && IsUpper(token[i]))) {
            if (tokensNumber != 0)
                splittedToken.push_back(' ');
            TUtf16String tokenToAdd = token.substr(j, i - j);
            if (!tokenToAdd.empty() && !tokenToAdd.StartsWith(WWW_PREFIX) && tokenToAdd.size() >= 2 && !StopWords.IsStopWord(tokenToAdd))
                tokenToAdd[0] = WordNormalizer->ToUpper(tokenToAdd[0]);
            splittedToken += tokenToAdd;
            ++tokensNumber;
            if (i < n)
                j = IsUpper(token[i]) ? i : i + 1;
       }
    }
    if (tokensNumber <= 1)
        return false;
    token = splittedToken;
    return true;
}

//Splitting token somehow (two basic options)
bool TUrlTitleTokenizer::TryToSplit(TUtf16String& token) {
    return TryToSplitBySeparators(token) || TryToSplitByDictionary(token);
}

bool TUrlTitleTokenizer::TokenLooksNatural(const TUtf16String& token) {
    size_t upperCnt = 0;
    for (size_t i = 0; i < token.size(); ++i)
        if (IsUpper(token[i]))
            ++upperCnt;
    return (upperCnt == token.size() || upperCnt < LOW_CAPITALS_RATIO * (token.size()));
}

TUrlTitleTokenizer::TUrlTitleTokenizer(const TQueryy& query, const TSentsInfo* sentsInfo, ELanguage lang, const TWordFilter& stopWords)
    : Query(query)
    , SentsInfo(sentsInfo)
    , WordNormalizer(NLemmer::GetAlphaRules(lang))
    , StopWords(stopWords)
{
}

//Trying to create artificial title from url using sizeable set of heuristics.
bool TUrlTitleTokenizer::GenerateUrlBasedTitle(TUtf16String& wurl, const TUtf16String& titleString) {
    const bool hostnameOnly = !titleString.empty();
    size_t n = wurl.size();
    size_t j = 0;
    bool firstSlash = true;
    TVector<TUtf16String> urlTitleTokens;
    for (size_t i = 0; i <= n; ++i) {
        if (i < n && !IsAlpha(wurl[i]) && wurl[i] != '.'
            && wurl[i] != '/' && wurl[i] != '-' && wurl[i] != '_' && wurl[i] != '?')
            return false;
        if (i == n || wurl[i] == '/' || wurl[i] == '.') {
            if (i < n && wurl[i] == '/' && firstSlash) {
                firstSlash = false;
            } else {
                if (i - j > MINIMAL_TOKEN_LENGTH)
                    urlTitleTokens.push_back(wurl.substr(j, i - j));
            }
            //ignoring the rest part of the url in some cases
            if (i < n && !firstSlash && wurl[i] == '.')
                break;
            if (i < n && wurl[i] == '?')
                break;
            j = i + 1;
        }
    }
    bool definitelyWithoutQueryWords = true; // heuristic for host name only calls, just trying to gain some time
    for (size_t i = 0; i < urlTitleTokens.size(); ++i) {
        if (!TokenLooksNatural(urlTitleTokens[i]))
            return false;
        if (!TryToSplit(urlTitleTokens[i])) {
            if (urlTitleTokens[i].size() > MAXIMAL_TOKEN_LENGTH)
                return false;
            if (hostnameOnly) {
                if (urlTitleTokens[i].find('-') != TUtf16String::npos || Query.LowerForms.contains(urlTitleTokens[i]))
                    definitelyWithoutQueryWords = false; // just became unsure because of hyphen or found at least one query word
            }
        } else {
            definitelyWithoutQueryWords = false; // not sure now
        }
        if (hostnameOnly && SimilarTitleStrings(titleString, urlTitleTokens[i], MIN_HOSTNAME_SIMILARITY_RATIO, true))
            return false;
        if (!urlTitleTokens[i].StartsWith(WWW_PREFIX)) {
            urlTitleTokens[i][0] = WordNormalizer->ToUpper(urlTitleTokens[i][0]);
        }
    }
    if (hostnameOnly && definitelyWithoutQueryWords)
        return false;
    wurl.clear();
    for (size_t i = 0; i < urlTitleTokens.size(); ++i) {
        if (i > 0)
            wurl += u" | ";
        wurl += urlTitleTokens[i];
    }
    return true;
}

TUtf16String ConvertUrlToTitle(const TString& url) {
    TString cutUrl = url;
    if (cutUrl.EndsWith('/')) {
        cutUrl.pop_back();
    }
    NUrlCutter::CutUrlPrefix(cutUrl);
    NUrlCutter::PreprocessUrl(cutUrl);
    return NUrlCutter::DecodeUrlHostAndPath(cutUrl);
}

}
