#pragma once
#include <library/cpp/containers/comptrie/comptrie.h>
#include <util/generic/string.h>
#include <util/generic/hash_set.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>

struct THyphenateParams {
    size_t MinSymbolsBeforeHyphenation = 0;
    size_t MinWordLengthForHyphenation = 0;
    THyphenateParams() = default;
    THyphenateParams(size_t minSymbolsBeforeHyphenation, size_t minWordLengthForHyphenation)
        : MinSymbolsBeforeHyphenation(minSymbolsBeforeHyphenation)
        , MinWordLengthForHyphenation(minWordLengthForHyphenation)
    {}
    ~THyphenateParams() = default;
};

class TWordHyphenator {
public:
    TString Hyphenate(const TString& str, const THyphenateParams& params = THyphenateParams()) const;
    TUtf16String Hyphenate(const TUtf16String& str, const THyphenateParams& params = THyphenateParams()) const;

private:
    using TTrie = TCompactTrie<wchar16, bool>;
    typedef bool (*HyphenateHandler)(const TUtf16String& str, size_t& cur_pos, TUtf16String& result, const THyphenateParams& params);

    static constexpr wchar16 SOFT_HYPHEN =  L'\u00AD';

    static THashSet<wchar16> RuByAlphabet;
    static THashSet<wchar16> RuByVowels;
    static THashSet<wchar16> RuByConsonants;
    static THashSet<wchar16> RuByVoicedConsonants;
    static THashSet<wchar16> RuByDeafConsonants;
    static THashSet<wchar16> RuJointVowels;
    static THashSet<wchar16> RuJointConsonants;
    static TTrie RuWordParts;

    static THashSet<wchar16> InitializeSet(TStringBuf arr);
    static TTrie InitializeTrie();
    static bool HasSymbol(const THashSet<wchar16>& set, wchar16 sumbol);
    static bool WordPartFound(const TTrie& trie);
    static bool RootFound(const TTrie& trie);

    static void RuGetHyphenRecommendationsBasedOnMorphAnalysis(const TUtf16String& str, size_t curPos, THashMap<size_t, bool>& result);
    static bool RuByHyphenate(const TUtf16String& str, size_t& curPos, TUtf16String& result, const THyphenateParams& params);

    TVector<HyphenateHandler> HyphenateHandlers = { RuByHyphenate };
};
