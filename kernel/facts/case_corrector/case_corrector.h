#pragma once

#include <library/cpp/containers/comptrie/comptrie.h>

#include <util/charset/wide.h>
#include <util/string/vector.h>
#include <util/generic/hash_set.h>

namespace NFacts {

    class TCaseCorrector {
    public:
        TCaseCorrector(TCompactTrie<char, TString>& decapsable, TCompactTrie<char, TString>& pretty);
        void Process(TString& text) const;
        static TVector<TUtf16String> SmartSplitIntoWords(const TUtf16String& sentence, TVector<std::pair<wchar16, size_t>>& highlightingTokenPos);

    private:
        static bool IsHelpingCharacter(wchar16 c);
        static bool AreHomogenousTokens(TVector<TUtf16String>::iterator begin, TVector<TUtf16String>::iterator end);
        static TUtf16String AssembleSentence(TVector<TUtf16String>& tokens);
        static TUtf16String InsertHighlightingTokens(const TUtf16String& sentence, const TVector<std::pair<wchar16, size_t>>& highlightingTokenPos);
        static void RemoveLastHighlightingToken(TString& sentence, bool& lastTokenHighlighting, char& highlightingToken);
        static void PreprocessTokens(TVector<TUtf16String>& tokens);
        void ApplyTrie(const TVector<TUtf16String>& tokens, TVector<TUtf16String>& src_tokens, const TCompactTrie<char, TString>& Trie) const;

        static const THashSet<wchar16> SENTENCE_ENDS;
        static const THashSet<wchar16> PUNCT_SET;
        static const THashSet<wchar16> NAMING_OPENING_QUOTES;
        static const THashSet<wchar16> NAMING_CLOSING_QUOTES;
        static const THashSet<wchar16> HIGHLIGHTING_TOKENS;
        TCompactTrie<char, TString> DecapsableTrie;
        TCompactTrie<char, TString> PrettyTrie;
    };

} // namespace NFacts
