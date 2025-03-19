#pragma once

#include "words_trie.h"

#include <library/cpp/binsaver/bin_saver.h>
#include <library/cpp/langmask/langmask.h>

#include <util/ysaveload.h>
#include <util/generic/hash_set.h>
#include <util/generic/maybe.h>
#include <util/generic/vector.h>

namespace NLemmasMerger {

class TLemmasMerger : public TCompactTrie<wchar16, ui32> {
private:
    using TBase = TCompactTrie<wchar16, ui32>;

    TWordsTrie Divisions;
    TVector<TUtf16String> Words;

    TLangMask LangMask;
public:
    TLemmasMerger() {
    }

    TLemmasMerger(const TLemmasMerger&) = default;

    TLemmasMerger(TLemmasMerger&& source)
        : TBase(std::move(source))
        , Divisions(std::move(source.Divisions))
        , Words(std::move(source.Words))
        , LangMask(source.LangMask)
    {
    }

    TLemmasMerger& operator = (const TLemmasMerger&) = default;
    TLemmasMerger& operator = (TLemmasMerger&& source) {
        if (this != &source) {
            TBase::operator=(std::move(source));

            Divisions = std::move(source.Divisions);
            Words = std::move(source.Words);
            LangMask = source.LangMask;
        }
        return *this;
    }

    void Load(IInputStream* in) {
        TString lemmasMergerBlob, divisionsBlob;
        ::Load(in, lemmasMergerBlob);
        ::Load(in, divisionsBlob);
        ::Load(in, Words);
        ::Load(in, LangMask);

        this->Init(TBlob::FromString(lemmasMergerBlob));
        Divisions.Init(TBlob::FromString(divisionsBlob));
    }

    void Save(IOutputStream* out) const {
        TString lemmasMergerBlob(this->Data().AsCharPtr(), this->Data().Size());
        TString divisionsBlob(Divisions.Data().AsCharPtr(), Divisions.Data().Size());

        ::Save(out, lemmasMergerBlob);
        ::Save(out, divisionsBlob);
        ::Save(out, Words);
        ::Save(out, LangMask);
    }

    int operator& (IBinSaver& f) {
        TStringStream saveloadBuffer;
        if (f.IsReading()) {
            f.Add(1, &saveloadBuffer.Str());
            Load(&saveloadBuffer);
        } else {
            Save(&saveloadBuffer);
            f.Add(1, &saveloadBuffer.Str());
        }
        return 0;
    }

    void LoadFromFile(const TString& filename) {
        TFileInput in(filename);
        Load(&in);
    }

    void SaveToFile(const TString& filename) const {
        TFixedBufferFileOutput out(filename);
        Save(&out);
    }

    void Build(const TVector<TString>& languageNames,
               const TVector<TString>& dictionaryFileNames,
               const TVector<TString>& derivationsFileNames);

    inline size_t GetWordsCount() const {
        return Words.size();
    }

    TMaybe<ui32> GetWordNumber(const wchar16* cleanedWordBegin, const wchar16* cleanedWordEnd) const {
        ui32 wordNumber;
        if (!this->Find(cleanedWordBegin, cleanedWordEnd - cleanedWordBegin, &wordNumber)) {
            return Nothing();
        }
        return TMaybe<ui32>(wordNumber);
    }

    TMaybe<ui32> GetWordNumber(const wchar16* cleanedWord, size_t lenght) const {
        return GetWordNumber(cleanedWord, cleanedWord + lenght);
    }

    TMaybe<ui32> GetWordNumber(const TUtf16String& word) const {
        TUtf16String cleanedWord = GetCleanedText(word);
        return GetWordNumber(cleanedWord.data(), cleanedWord.size());
    }

    const TUtf16String& GetWordByNumber(ui32 wordNumber) const {
        return Words[wordNumber];
    }

    void ReadHashes(TVector<ui64>& unigramHashes,
                    const TUtf16String& text,
                    size_t hashesCountLimit = 0) const;
    void ReadHashesClean(TVector<ui64>& unigramHashes,
                         const TUtf16String& text,
                         size_t wordCountLimit = 0) const;

    TVector<ui64> ReadHashes(const TUtf16String& text, size_t wordCountLimit = 0) const;
    TVector<ui32> ReadText(const TUtf16String& text, size_t wordCountLimit = 0, bool minimizeDivisions = true) const;

    static TUtf16String GetCleanedText(const TUtf16String& text);

    ui64 Hash(const ui32 wordNumber) const;
private:
    void AddDivisionHypothesises(const TVector<THashSet<ui32> >& derivations,
                                 size_t wordNumber,
                                 TCompactTrieBuilder<ui32, ui32>& divisionsBuilder);
    void BuildDivisions(const TVector<THashSet<ui32> >& derivations);

    void ReadDictionaries(const TVector<TString>& dictionaryFileNames);
    void ReadDictionary(const TString& dictFileName,
                        TVector<ui32>& parents,
                        TVector<ui32>& ranks,
                        TCompactTrieBuilder<wchar16, ui32>& lemmasBaseNumbersBuilder);

    void ReadDerivations(TVector<THashSet<ui32> >& derivations, const TString& derivationsFileName);

    ui32 GetRoot(ui32 number, TVector<ui32>& parents) const;
    void Unite(ui32 left, ui32 right, TVector<ui32>& parents, TVector<ui32>& ranks) const;
};

}
