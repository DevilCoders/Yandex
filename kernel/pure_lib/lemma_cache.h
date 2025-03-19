#pragma once

#include <kernel/search_types/search_types.h>
#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>

#include <kernel/lemmer/core/language.h>
#include <library/cpp/tokenizer/tokenizer.h>

#include <ysite/yandex/common/prepattr.h>

class TSimpleLemmatizationCache {
private:
    typedef TVector<TString> TForms;
    typedef THashMap<TUtf16String, TForms> TData;
    TData Data;
    bool PureMode;
    size_t CacheHits;
    bool Verbose;

    struct TCmpByQuality {
        bool operator()(const TYandexLemma& lhs, const TYandexLemma& rhs) {
            // if (lhs.IsBest() != rhs.IsBest())
            //    return lhs.IsBest() > rhs.IsBest();
            if (lhs.GetQuality() != rhs.GetQuality())
                return lhs.GetQuality() < rhs.GetQuality();
            return lhs.GetLanguage() < rhs.GetLanguage();
        }
    };

public:
    TSimpleLemmatizationCache(bool pureMode, bool verbose = false)
        : PureMode(pureMode)
        , CacheHits(0)
        , Verbose(verbose)
    {
    }

    void SimpleLemmatize_(const wchar16* dstToken, size_t dstLen, NLP_TYPE type, TLangMask langFlags, TVector<TString>* result) {
        wchar16 prepared[MAXKEY_BUF];
        size_t preparedLen = 0;
        if (type != NLP_WORD)
            preparedLen = PrepareNonLemmerToken(type, dstToken, prepared);
        TWLemmaArray lemmas;
        if ((0 == preparedLen) || (!PureMode && NLP_INTEGER == type))
            NLemmer::AnalyzeWord(dstToken, dstLen, lemmas, langFlags);
        else
            NLemmer::AnalyzeWord(prepared, preparedLen, lemmas, langFlags);
        THashSet<TString> used;
        Sort(lemmas.begin(), lemmas.end(), TCmpByQuality());
        for (size_t i = 0; i < lemmas.size(); ++i) {
            const TUtf16String wLemma = TUtf16String(lemmas[i].GetText(), lemmas[i].GetTextLength());
            TString lemma = WideToUTF8(wLemma);
            if (used.find(lemma) == used.end()) {
                result->push_back(lemma);
                used.insert(lemma);
            }
        }
        if (used.empty())
            result->push_back( WideToUTF8( TUtf16String(dstToken, dstLen) ) );
    }

    const TVector<TString>& SimpleLemmatize(const wchar16* dstToken, size_t dstLen, NLP_TYPE type, TLangMask langFlags) {
        static const TUtf16String DELIM = u"_";
        TUtf16String key(dstToken, dstLen);
        key += DELIM;
        key += ASCIIToWide( ToString((int)type) );
        key += DELIM;
        key += ASCIIToWide( langFlags.ToString() );
        TData::const_iterator toKey = Data.find(key);
        if (toKey == Data.end()) {
            if (Data.size() > 500000) {
                if (Verbose)
                    Cerr << "SimpleLemmatizationCache: " << Data.size() << "\t" << CacheHits << Endl;
                CacheHits = 0;
                Data.clear();
            }
            SimpleLemmatize_(dstToken, dstLen, type, langFlags, &Data[key]);
            toKey = Data.find(key);
        } else {
            ++CacheHits;
        }
        return toKey->second;
    }
};
