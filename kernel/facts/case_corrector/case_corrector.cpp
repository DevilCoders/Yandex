#include "case_corrector.h"

#include <quality/functionality/facts/tools/sentence_split/cpp/split.h>

#include <util/generic/algorithm.h>

namespace NFacts {
    const THashSet<wchar16> TCaseCorrector::SENTENCE_ENDS = {u'.', u'!', u'?'};
    const THashSet<wchar16> TCaseCorrector::PUNCT_SET = {u'"', u'#', u'$', u'%', u'&', u'\'', u'(', u')', u'*', u'+', u',', u'/', u':', u';', u'<', u'=', u'>', u'@', u'[', u'\\', u']', u'^', u'_', u'`', u'{', u'|', u'}', u'~'};
    const THashSet<wchar16> TCaseCorrector::NAMING_OPENING_QUOTES = {u'"', u'\'', u'«'};
    const THashSet<wchar16> TCaseCorrector::NAMING_CLOSING_QUOTES = {u'"', u'\'', u'»'};
    const THashSet<wchar16> TCaseCorrector::HIGHLIGHTING_TOKENS = {'\x07', u'[', u']'};

    TCaseCorrector::TCaseCorrector(TCompactTrie<char, TString>& decapsable, TCompactTrie<char, TString>& pretty)
        : DecapsableTrie(decapsable)
        , PrettyTrie(pretty)
    {
    }

    void TCaseCorrector::Process(TString& text) const {
        TVector<TString> sentences = SmartSplitIntoSentences(text);

        bool lastTokenHighlighting = false;
        char highlightingToken = '\x07';
        TVector<TUtf16String> dstSentences;

        for (TVector<TString>::iterator it = sentences.begin(); it != sentences.end(); ++it) {
            if (lastTokenHighlighting)
                it->insert(it->begin(), 1, highlightingToken);

            if (it != sentences.end() - 1)
                RemoveLastHighlightingToken(*it, lastTokenHighlighting, highlightingToken);

            TVector<std::pair<wchar16, size_t>> highlightingTokenPos;
            TVector<TUtf16String> words = SmartSplitIntoWords(UTF8ToWide(*it), highlightingTokenPos);
            if (words.empty() && highlightingTokenPos.empty())
                continue;
            PreprocessTokens(words);

            TVector<TUtf16String> decapsedWords = words;
            ApplyTrie(words, decapsedWords, DecapsableTrie);
            TVector<TUtf16String> lowerWords = decapsedWords;
            for (TUtf16String& word : lowerWords) {
                word.to_lower();
            }

            TVector<TUtf16String> prettifiedWords = decapsedWords;
            ApplyTrie(lowerWords, prettifiedWords, PrettyTrie);
            TUtf16String assembledSentence = AssembleSentence(prettifiedWords);
            TUtf16String newSentence = InsertHighlightingTokens(assembledSentence, highlightingTokenPos);
            if (!newSentence.empty()) {
                dstSentences.push_back(newSentence);
            }
        }

        text = WideToUTF8(JoinStrings(dstSentences, u" "));
    }

    bool TCaseCorrector::IsHelpingCharacter(wchar16 c) {
        return (
            SENTENCE_ENDS.contains(c) ||
            PUNCT_SET.contains(c) ||
            NAMING_OPENING_QUOTES.contains(c) ||
            NAMING_CLOSING_QUOTES.contains(c));
    }

    bool TCaseCorrector::AreHomogenousTokens(TVector<TUtf16String>::iterator begin, TVector<TUtf16String>::iterator end) {
        bool gotNonascii = false;

        for (auto it = begin; it != end; ++it) {
            bool allAlpha = AllOf(*it, IsAlpha);
            bool allAscii = AllOf(*it, [](const wchar16& c) { return c < 128; });

            if (!allAlpha)
                continue;
            if (allAscii)
                return false;
            else
                gotNonascii = true;
        }

        return gotNonascii;
    }

    TUtf16String TCaseCorrector::AssembleSentence(TVector<TUtf16String>& tokens) {
        if (tokens.empty())
            return u"";
        if (tokens[0] == u"")
            return u"";
        if (tokens.size() == 1 && SENTENCE_ENDS.contains(tokens[0][0]))
            return u"";
        tokens[0][0] = ToUpper(tokens[0][0]);
        return JoinStrings(tokens, u"");
    }

    TUtf16String TCaseCorrector::InsertHighlightingTokens(const TUtf16String& sentence, const TVector<std::pair<wchar16, size_t>>& highlightingTokenPos) {
        TUtf16String newSentence = TUtf16String(sentence.length() + highlightingTokenPos.size(), '\x07');
        size_t curPos = 0;
        size_t ptrSentence = 0;
        size_t ptrHighlightingTokenPos = 0;
        while (ptrSentence < sentence.length() && ptrHighlightingTokenPos < highlightingTokenPos.size()) {
            if (curPos != highlightingTokenPos[ptrHighlightingTokenPos].second) {
                newSentence[curPos] = sentence[ptrSentence++];
            } else {
                newSentence[curPos] = highlightingTokenPos[ptrHighlightingTokenPos++].first;
            }
            ++curPos;
        }

        if (ptrSentence >= sentence.length()) {
            while (ptrHighlightingTokenPos < highlightingTokenPos.size())
                newSentence[curPos++] = highlightingTokenPos[ptrHighlightingTokenPos++].first;
        } else {
            while (ptrSentence < sentence.length())
                newSentence[curPos++] = sentence[ptrSentence++];
        }

        return newSentence;
    }

    void TCaseCorrector::RemoveLastHighlightingToken(TString& sentence, bool& lastTokenHighlighting, char& highlightingToken) {
        if (sentence.empty())
            return;

        if (HIGHLIGHTING_TOKENS.contains(sentence.back())) {
            lastTokenHighlighting = true;
            highlightingToken = sentence.back();
            sentence.pop_back();
        } else {
            lastTokenHighlighting = false;
        }
    }

    TVector<TUtf16String> TCaseCorrector::SmartSplitIntoWords(const TUtf16String& sentence, TVector<std::pair<wchar16, size_t>>& highlightingTokenPos) {
        TVector<TUtf16String> tokens;
        size_t curPos = 0;
        for (wchar16 c : sentence) {
            ++curPos;
            if (IsSpace(c)) {
                if (!tokens.empty() && !IsSpace(tokens.back())) {
                    tokens.push_back(u" ");
                } else {
                    --curPos;
                }
            } else if (HIGHLIGHTING_TOKENS.contains(c)) {
                highlightingTokenPos.emplace_back(c, curPos - 1);
            } else if (IsHelpingCharacter(c)) {
                tokens.push_back(TUtf16String(c));
            } else if (!tokens.empty() && !IsSpace(tokens.back()) && !IsHelpingCharacter(tokens.back().back())) {
                tokens.back() += c;
            } else {
                tokens.push_back(TUtf16String(c));
            }
        }
        if (!tokens.empty() && IsSpace(tokens.back())) {
            tokens.pop_back();
        }

        return tokens;
    }

    void TCaseCorrector::PreprocessTokens(TVector<TUtf16String>& tokens) {
        TUtf16String prevToken = u" ";
        bool isNamingScope = false;

        for (TUtf16String& token : tokens) {
            bool anyAlnum = AnyOf(token, IsAlnum);
            bool allCapsed = AllOf(token, [](const wchar16& c) { return !IsAlnum(c) || IsUpper(c); });
            bool lastTokenWord = AllOf(token, [](const wchar16& c) { return !IsSpace(c) && !PUNCT_SET.contains(c); });

            if (anyAlnum && allCapsed && NAMING_OPENING_QUOTES.contains(prevToken.back())) {
                token.to_lower();
                token[0] = ToUpper(token[0]);
                isNamingScope = true;
            } else if (isNamingScope && NAMING_CLOSING_QUOTES.contains(token.back()) && lastTokenWord) {
                isNamingScope = false;
            } else if (isNamingScope && allCapsed) {
                token.to_lower();
            }
            prevToken = token;
        }

        bool capsStarted = false;
        size_t capsStartPos = 0;
        int numMeaningfulTokens = 0;

        for (size_t i = 0; i <= tokens.size(); i++) {
            if (i < tokens.size() && (IsUpperWord(tokens[i]) || IsSpace(tokens[i]))) {
                if (!capsStarted) {
                    capsStartPos = i;
                    capsStarted = true;
                }
                numMeaningfulTokens += IsUpperWord(tokens[i]) ? 1 : 0;
            } else if (capsStarted) {
                if (numMeaningfulTokens > 1 && AreHomogenousTokens(tokens.begin() + capsStartPos, tokens.begin() + i)) {
                    for (size_t j = capsStartPos; j < i; j++) {
                        tokens[j].to_lower();
                    }
                }
                capsStarted = false;
                numMeaningfulTokens = 0;
            }
        }
    }

    void TCaseCorrector::ApplyTrie(const TVector<TUtf16String>& tokens, TVector<TUtf16String>& dstTokens, const TCompactTrie<char, TString>& Trie) const {
        size_t candidateStart = 0;
        TVector<std::pair<wchar16, size_t>> dummyHighlightingTokenPos;
        while (candidateStart < tokens.size()) {
            std::pair<TString, size_t> bestSubstitution = {"", 0};
            TUtf16String candidate = u"";
            for (size_t i = candidateStart; i < tokens.size(); i++) {
                if (IsSpace(tokens[i])) {
                    if (candidate == u"")
                        candidateStart++;
                    else
                        candidate += tokens[i];
                    continue;
                }
                candidate += tokens[i];
                TString query = WideToUTF8(candidate);

                TString leafValue = "";
                bool found = Trie.Find(query, &leafValue);

                if (found && !leafValue.empty()) {
                    bestSubstitution = {leafValue, i + 1};
                }
                if (!found) {
                    break;
                }
            }

            if (!bestSubstitution.first.empty()) {
                TVector<TUtf16String> substitutionTokens = SmartSplitIntoWords(UTF8ToWide(bestSubstitution.first), dummyHighlightingTokenPos);
                for (size_t i = candidateStart; i < bestSubstitution.second; i++)
                    dstTokens[i] = substitutionTokens[i - candidateStart];
                candidateStart = bestSubstitution.second;
                bestSubstitution = {"", 0};
            } else {
                candidateStart++;
            }
        }
    }
}
