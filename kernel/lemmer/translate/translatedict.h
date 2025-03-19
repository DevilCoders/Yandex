#pragma once

#include <kernel/lemmer/dictlib/grambitset.h>
#include <util/charset/wide.h>
#include <util/generic/vector.h>

class IInputStream;

class TTranslationDict {
public:
    struct TArticle {
        const wchar16* Word;
        TGramBitSet Grammar;

        TArticle(const wchar16* word, const TGramBitSet& grammar = TGramBitSet())
            : Word(word)
            , Grammar(grammar)
        {
        }
    };

public:
    TTranslationDict();
    ~TTranslationDict();
    size_t FromEnglish(const TArticle& art, TVector<TArticle>& result) const;
    size_t ToEnglish(const TArticle& art, TVector<TArticle>& result) const;
    void Load(IInputStream& inputStream);

private:
    class TImpl;
    THolder<TImpl> Impl;
};
