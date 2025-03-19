#include "lemmas_merger.h"

#include <library/cpp/disjoint_sets/disjoint_sets.h>
#include <library/cpp/tokenizer/tokenizer.h>
#include <library/cpp/token/charfilter.h>

#include <util/generic/hash.h>
#include <util/string/split.h>
#include <util/charset/wide.h>

namespace NLemmasMerger {

template <typename TKey, typename TValue>
void SaveCompactTrie(const TCompactTrieBuilder<TKey, TValue>& builder, TCompactTrie<TKey, TValue>& target) {
    TBufferOutput raw;
    builder.Save(raw);

    TBufferOutput compacted;
    CompactTrieMinimize<TCompactTrie<>::TPacker>(compacted, raw.Buffer().Data(), raw.Buffer().Size(), false);

    target.Init(TBlob::FromBuffer(compacted.Buffer()));
}

void TLemmasMerger::AddDivisionHypothesises(const TVector<THashSet<ui32> >& derivations,
                                            size_t wordNumber,
                                            TCompactTrieBuilder<ui32, ui32>& divisionsBuilder)
{
    const TUtf16String& word = Words[wordNumber];

    TVector<ui32> hypothesis(2);
    auto addDerivationHypothesises = [&](const THashSet<ui32>& theDerivations) {
        for (const ui32 derivation : theDerivations) {
            hypothesis[1] = derivation;
            divisionsBuilder.Add(hypothesis.data(), hypothesis.size(), wordNumber);
        }
    };

    for (const wchar16* delimeter = word.begin() + 3; delimeter + 3 < word.end(); ++delimeter) {
        TMaybe<ui32> left = GetWordNumber(word.begin(), delimeter);
        TMaybe<ui32> right = GetWordNumber(delimeter, word.end());

        if (left.Empty() || right.Empty()) {
            continue;
        }

        hypothesis[0] = *left;
        hypothesis[1] = *right;

        divisionsBuilder.Add(hypothesis.data(), hypothesis.size(), wordNumber);

        addDerivationHypothesises(derivations[*right]);
        for (const ui32 leftDerivation : derivations[*left]) {
            hypothesis[0] = leftDerivation;
            addDerivationHypothesises(derivations[*right]);
        }
    }
}

void TLemmasMerger::BuildDivisions(const TVector<THashSet<ui32> >& derivations) {
    TCompactTrieBuilder<ui32, ui32> divisionsBuilder;

    for (ui32 wordNumber = 0; wordNumber < Words.size(); ++wordNumber) {
        AddDivisionHypothesises(derivations, wordNumber, divisionsBuilder);
    }

    SaveCompactTrie(divisionsBuilder, Divisions);
}

void TLemmasMerger::ReadDictionaries(const TVector<TString>& dictionaryFileNames) {
    TVector<ui32> parents, ranks;

    TCompactTrieBuilder<wchar16, ui32> lemmasBaseNumbersBuilder;
    for (const TString& dictionaryFileName : dictionaryFileNames) {
        ReadDictionary(dictionaryFileName, parents, ranks, lemmasBaseNumbersBuilder);
    }
    SaveCompactTrie(lemmasBaseNumbersBuilder, *this);

    TVector<TUtf16String> actualBaseWords;
    THashMap<ui32, ui32> parentsMap;
    for (size_t wordNumber = 0; wordNumber < Words.size(); ++wordNumber) {
        ui32 root = GetRoot(wordNumber, parents);
        if (!parentsMap.contains(root)) {
            size_t actualWordsCount = actualBaseWords.size();
            parentsMap[root] = actualWordsCount;
            actualBaseWords.push_back(Words[root]);
        }
    }
    actualBaseWords.swap(Words);

    TCompactTrieBuilder<wchar16, ui32> actualLemmasBaseNumbersBuilder;
    for (auto&& wordWithNumber : *this) {
        const TUtf16String& word = wordWithNumber.first;

        const ui32 wordNumber = wordWithNumber.second;
        const ui32 parent = parentsMap[GetRoot(wordNumber, parents)];

        actualLemmasBaseNumbersBuilder.Add(word, parent);
    }
    SaveCompactTrie(actualLemmasBaseNumbersBuilder, *this);
}

void TLemmasMerger::ReadDictionary(const TString& dictFileName,
                                   TVector<ui32>& parents,
                                   TVector<ui32>& ranks,
                                   TCompactTrieBuilder<wchar16, ui32>& lemmasBaseNumbersBuilder)
{
    if (!dictFileName) {
        return;
    }

    TFileInput in(dictFileName);
    TString dataStr;

    ui32 baseWordNumber = (ui32) -1;
    while (in.ReadLine(dataStr)) {
        if (!dataStr) {
            continue;
        }
        TStringBuf dataStrBuf(dataStr);

        TVector<TStringBuf> split;
        StringSplitter(dataStrBuf.begin(), dataStrBuf.end()).Split(' ').AddTo(&split);

        TUtf16String word;
        if (dataStrBuf[0] != '[') {
            TStringBuf wordInfoBuf = split[0];
            if (wordInfoBuf[0] == '@') {
                wordInfoBuf = split[1];
            }

            TStringBuf wordBuf, restBuf;
            wordInfoBuf.Split('_', wordBuf, restBuf);
            word = UTF8ToWide(wordBuf);
        } else {
            word = UTF8ToWide(split[0]);
        }

        word.to_lower();
        word.erase(word.find('['), 1);
        word.erase(word.find(']'), 1);

        if (!word) {
            continue;
        }

        word = GetCleanedText(word);

        if (dataStrBuf[0] == '[' || (dataStrBuf[0] == '@' && split[0] != "@ID:")) {
            ui32 existedBaseWordNumber;
            if (lemmasBaseNumbersBuilder.Find(word.data(), word.size(), &existedBaseWordNumber)) {
                Unite(existedBaseWordNumber, baseWordNumber, parents, ranks);
            } else {
                lemmasBaseNumbersBuilder.Add(word.data(), word.size(), baseWordNumber);
            }
        } else if (!lemmasBaseNumbersBuilder.Find(word.data(), word.size(), &baseWordNumber)) {
            baseWordNumber = Words.size();

            lemmasBaseNumbersBuilder.Add(word.data(), word.size(), baseWordNumber);
            Words.push_back(word);

            parents.push_back(baseWordNumber);
            ranks.push_back(0);
        }
    }
}

void TLemmasMerger::ReadDerivations(TVector<THashSet<ui32> >& derivations, const TString& derivationsFileName) {
    if (!derivationsFileName) {
        return;
    }

    TFileInput in(derivationsFileName);
    TString dataStr;
    while (in.ReadLine(dataStr)) {
        TStringBuf dataStrBuf(dataStr);

        TStringBuf left, right;
        dataStrBuf.Split('\t', left, right);

        if (!left || !right) {
            continue;
        }

        TUtf16String leftWord = GetCleanedText(UTF8ToWide(left));
        TUtf16String rightWord = GetCleanedText(UTF8ToWide(right));

        ui32 leftBaseWordNumber, rightBaseWordNumber;
        if (!this->Find(leftWord.data(), leftWord.size(), &leftBaseWordNumber) ||
            !this->Find(rightWord.data(), rightWord.size(), &rightBaseWordNumber))
        {
            continue;
        }

        derivations[leftBaseWordNumber].insert(rightBaseWordNumber);
        derivations[rightBaseWordNumber].insert(leftBaseWordNumber);
    }
}

ui32 TLemmasMerger::GetRoot(ui32 number, TVector<ui32>& parents) const {
    ui32 root;
    ui32 current = number;

    while (parents[current] != current) {
        current = parents[current];
    }

    root = current;

    current = number;
    while (parents[current] != current) {
        parents[current] = root;
        current = parents[current];
    }

    return root;
}

void TLemmasMerger::Unite(ui32 left, ui32 right, TVector<ui32>& parents, TVector<ui32>& ranks) const {
    ui32 leftRoot = GetRoot(left, parents);
    ui32 rightRoot = GetRoot(right, parents);

    if (leftRoot == rightRoot) {
        return;
    }

    ui32 newRoot = ranks[leftRoot] >= ranks[rightRoot] ? leftRoot : rightRoot;
    parents[leftRoot] = newRoot;
    parents[rightRoot] = newRoot;

    if (ranks[leftRoot] == ranks[rightRoot]) {
        ++ranks[newRoot];
    }
}

namespace {

class TLemmasMergerTokenHandler : public ITokenHandler {
private:
    TVector<ui32>& Words;
    const TLemmasMerger& LemmasMerger;
    size_t WordCountLimit;
public:
    TLemmasMergerTokenHandler(TVector<ui32>& words,
                              const TLemmasMerger& lemmasMerger,
                              size_t wordCountLimit = 0)
        : Words(words)
        , LemmasMerger(lemmasMerger)
        , WordCountLimit(wordCountLimit)
    {
        Words.clear();
    }

    void OnToken(const TWideToken& token, size_t, NLP_TYPE type) override {
        if (type != NLP_WORD || (WordCountLimit && Words.size() > WordCountLimit)) {
            return;
        }

        if (token.SubTokens.size() > 1) {
            TMaybe<ui32> wordNumber = LemmasMerger.GetWordNumber(token.Token, token.Leng);
            if (wordNumber.Defined()) {
                Words.push_back(*wordNumber);
                return;
            }
        }

        for (const TCharSpan& subtoken : token.SubTokens) {
            TMaybe<ui32> wordNumber = LemmasMerger.GetWordNumber(token.Token + subtoken.Pos, subtoken.Len);
            if (wordNumber.Defined()) {
                Words.push_back(*wordNumber);
            }
        }
    }
};

class TLemmasMergerHashesTokenHandler : public ITokenHandler {
private:
    TVector<ui64>& UnigramHashes;

    const TLemmasMerger& LemmasMerger;

    size_t HashesCountLimit;
public:
    TLemmasMergerHashesTokenHandler(TVector<ui64>& unigramHashes,
                                    const TLemmasMerger& lemmasMerger,
                                    size_t hashesCountLimit = 0)
        : UnigramHashes(unigramHashes)
        , LemmasMerger(lemmasMerger)
        , HashesCountLimit(hashesCountLimit)
    {
        UnigramHashes.clear();
    }

    void OnToken(const TWideToken& token, size_t, NLP_TYPE type) override {
        if (!EqualToOneOf(type, NLP_WORD, NLP_MARK, NLP_MISCTEXT) ||
            (HashesCountLimit && UnigramHashes.size() >= HashesCountLimit))
        {
            return;
        }

        if (token.SubTokens.size() > 1) {
            TMaybe<ui32> wordNumber = LemmasMerger.GetWordNumber(token.Token, token.Leng);
            if (wordNumber.Defined()) {
                AppendHash(LemmasMerger.Hash(*wordNumber));
                return;
            }
        }

        for (const TCharSpan& subtoken : token.SubTokens) {
            TMaybe<ui32> wordNumber = LemmasMerger.GetWordNumber(token.Token + subtoken.Pos, subtoken.Len);
            ui64 hash = wordNumber.Defined() ? LemmasMerger.Hash(*wordNumber)
                                             : ComputeHash(TWtringBuf(token.Token + subtoken.Pos, subtoken.Len));
            AppendHash(hash);
        }
    }
private:
    void AppendHash(const ui64 hash) {
        UnigramHashes.push_back(hash);
    }
};

}

TVector<ui32> TLemmasMerger::ReadText(const TUtf16String& text, size_t wordCountLimit, bool minimizeDivisions) const {
    TVector<ui32> words;

    TUtf16String normalizedText = GetCleanedText(text);

    TLemmasMergerTokenHandler processor(words, *this, wordCountLimit);
    TNlpTokenizer(processor).Tokenize(normalizedText.data(), normalizedText.size());

    if (minimizeDivisions) {
        Divisions.Minimize(words);
    }

    return words;
}

void TLemmasMerger::ReadHashesClean(TVector<ui64>& unigramHashes,
                                    const TUtf16String& text,
                                    size_t wordCountLimit) const
{
    TUtf16String normalizedText = GetCleanedText(text);

    TLemmasMergerHashesTokenHandler processor(unigramHashes, *this, wordCountLimit);
    TNlpTokenizer(processor).Tokenize(normalizedText.data(), normalizedText.size());
}


void TLemmasMerger::ReadHashes(TVector<ui64>& unigramHashes,
                               const TUtf16String& text,
                               size_t wordCountLimit) const
{
    return ReadHashesClean(unigramHashes, GetCleanedText(text), wordCountLimit);
}

TVector<ui64> TLemmasMerger::ReadHashes(const TUtf16String& text, size_t wordCountLimit) const {
    TVector<ui64> hashes;
    ReadHashes(hashes, text, wordCountLimit);
    return hashes;
}

void TLemmasMerger::Build(const TVector<TString>& languageNames,
                          const TVector<TString>& dictionaryFileNames,
                          const TVector<TString>& derivationsFileNames)
{
    LangMask = TLangMask();
    for (const TString& languageName : languageNames) {
        LangMask.SafeSet(LanguageByName(languageName));
    }

    TVector<TUtf16String>().swap(Words);
    ReadDictionaries(dictionaryFileNames);

    TVector<THashSet<ui32> > derivations(Words.size());
    for (const TString& derivationsFileName : derivationsFileNames) {
        ReadDerivations(derivations, derivationsFileName);
    }
    BuildDivisions(derivations);
}

TUtf16String TLemmasMerger::GetCleanedText(const TUtf16String& text) {
    // See https://wiki.yandex-team.ru/poiskovajaplatforma/lingvistika/removediacritics/
    TUtf16String mappedString = NormalizeUnicode(text);

    wchar16* writePointer = mappedString.begin();
    const wchar16* readPointer = mappedString.begin();
    while (readPointer != mappedString.end()) {
        if (IsQuotation(*readPointer)) {
            ++readPointer;
            continue;
        }

        *writePointer = *readPointer;
        ++writePointer;
        ++readPointer;
    }

    mappedString.erase(writePointer, mappedString.end());

    return mappedString;
}

ui64 TLemmasMerger::Hash(const ui32 wordNumber) const {
    return IntHash<ui64>(wordNumber);
}

}
