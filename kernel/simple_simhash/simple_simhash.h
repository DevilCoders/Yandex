#pragma once

#include <library/cpp/numerator/numerate.h>
#include <library/cpp/lrucache/lrucache.h>
#include <ysite/yandex/pure/pure_container.h>


const size_t BIT_COUNT = 64;

class TFreqs {
public:
    ui64 LemmaHash;
    double Idf;
    ui32 Count;

public:
    TFreqs() {
    }

    TFreqs(ui64 lemmaHash, double idf)
        : LemmaHash(lemmaHash)
        , Idf(idf)
        , Count(0)
    {
    }
};

class TFreqsDoc {
public:
    ui64 LemmaHash;
    double Idf;
    ui32 Doc = 0;
    ui32 Index;
};

class TSimpleSimhashHandler : public INumeratorHandler, private TNonCopyable {
public:
    TSimpleSimhashHandler();

    void Reset(TSimpleSharedPtr<TPureContainer> pureContainer, const TLangMask& langMask);

    void OnTokenStart(const TWideToken& tok, const TNumerStat& /*stat*/) override;

    ui64 GetSimhash() const;
private:
    void ProcessWord(const wchar16* word, ui32 len);

    TSimpleSharedPtr<TPureContainer> PureContainer;
    TLangMask LangMask;

    ui64 TotalCollectionLength;
    TLRUCache<ui64, TFreqsDoc> FreqCache;
    ui32 Doc = 1;
    TVector<TFreqs> Freqs;
};
