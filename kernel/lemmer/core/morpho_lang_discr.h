#pragma once

#include <library/cpp/langmask/langmask.h>
#include <util/generic/map.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>

class TWordInstance;

namespace NMorphoLangDiscriminator {
    enum TQuality {
        QNo = 0,
        QBad = 1,
        QAverage,
        QPref,
        QAlmostGood,
        QGood,
        QVeryGood,
        QName,
        QGoodFreq
    };
    typedef TMap<ELanguage, TQuality> TLang2Qual;

    class TContext {
        class TContextLoader;
        friend class TContextLoader;

    public:
        struct TVQual {
            TQuality val[LANG_MAX];

            TVQual() {
                memset(val, 0, sizeof(val));
            }
            TQuality operator[](int i) const {
                return val[i];
            }
            TQuality& operator[](int i) {
                return val[i];
            }
        };

    private:
        static const TVQual EmptyQual;

        typedef THashMap<TUtf16String, TVQual> TStr2Lang;
        TStr2Lang Words;

    public:
        static const TContext EmptyContext;

    private:
        void ParseLine(const TUtf16String& line, ELanguage langcode, int version);

    public:
        inline TContext() {
        }

        bool Init(const char* fileName);
        bool Init(IInputStream& dataFile);

        void Term() {
            Words.clear();
        }

        const TVQual& GetMask(const TUtf16String& word) const {
            TStr2Lang::const_iterator it = Words.find(word);
            if (it != Words.end())
                return it->second;
            return EmptyQual;
        }
    };

    void GetQuality(const TWordInstance& word, TLang2Qual& ret, const TContext& ctx);
}

class TMorphoLangDiscriminator {
public:
    struct TResult {
        TLangMask Loosers;
        TLangMask Winners;
        TResult(TLangMask loosers, TLangMask winners)
            : Loosers(loosers)
            , Winners(winners)
        {
        }
    };

private:
    const NMorphoLangDiscriminator::TContext& Context;
    ui16 Diamonds[LANG_MAX][LANG_MAX];
    ui16 Rubies[LANG_MAX][LANG_MAX];
    ui16 Intersection[LANG_MAX][LANG_MAX];
    TLangMask IntersectedLanguages;
    TLangMask AllLanguages;
    const TLangMask KeepLanguages;
    THashSet<TUtf16String> Words;

private:
    void AddDiamonds(const NMorphoLangDiscriminator::TLang2Qual& qual, const TUtf16String& form, bool isTheOnlyWord);

public:
    explicit TMorphoLangDiscriminator(const NMorphoLangDiscriminator::TContext& ctx = NMorphoLangDiscriminator::TContext::EmptyContext, TLangMask keepLanguages = TLangMask());
    void AddWord(const TWordInstance& word, bool isTheOnlyWord);
    TResult ObtainResult(TLangMask preferedLanguage = TLangMask()) const;
};
