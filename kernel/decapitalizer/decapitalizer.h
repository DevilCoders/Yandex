#pragma once

#include <library/cpp/langmask/langmask.h>
#include <library/cpp/langs/langs.h>
#include <kernel/lemmer/alpha/abc.h>
#include <library/cpp/tokenizer/tokenizer.h>
#include <library/cpp/tokenizer/split.h>

#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/singleton.h>
#include <util/generic/string.h>
#include <util/generic/vector.cpp>
#include <util/charset/wide.h>

namespace NDecapitalizer {
    extern const TLangMask DEFAULT_LANG_MASK;
    extern const ELanguage DEFAULT_MAIN_LANGUAGE;

    struct THintInfo {
        size_t RegularCount = 0;
        size_t CamelCount = 0;
        size_t UpperCount = 0;

        void operator+=(const THintInfo& diff);
    };

    class TDecapitalizerHint: public ITokenHandler {
    private:
        const TLangMask& Langs;
        THashMap<TUtf16String, THintInfo> Hints;
        bool IsFirstWord = true;
        void AddWord(const TWtringBuf& word);

    public:
        TDecapitalizerHint(const TLangMask& langs);
        void OnToken(const TWideToken& token, size_t, NLP_TYPE type) override;
        THintInfo GetInfo(const TWtringBuf& word) const;
        void StartNewHint();
    };

    class TDecapitalizer: public ITokenHandler {
    private:
        const THashSet<TUtf16String>& Abbrevs;
        TDecapitalizerHint Hint;
        const NLemmer::TAlphabetWordNormalizer* Normalizer = nullptr;
        const TTokenizerSplitParams& Params;
        bool IsFirstWord = true;
        bool PrvDecapitalized = false;
        TUtf16String* Text = nullptr;

    public:
        TDecapitalizer(const THashSet<TUtf16String>& abbrevs, const TVector<TUtf16String>& hints,
                       ELanguage MainLanguage = DEFAULT_MAIN_LANGUAGE, const TLangMask& langs = DEFAULT_LANG_MASK,
                       const TTokenizerSplitParams& params = *Singleton<TTokenizerSplitParams>());

        void OnToken(const TWideToken& token, size_t, NLP_TYPE type) override;
        void Decapitalize(TUtf16String& text);

    private:
        bool DecapitalizeWord(TWtringBuf& word);
        bool DecapitalizeByHint(TWtringBuf& word, const THintInfo& hintInfo);
        void ToLower(const TWtringBuf& word, bool skipFirst = false);
    };
}
