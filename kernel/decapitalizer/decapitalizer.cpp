#include "decapitalizer.h"

#include <kernel/lemmer/core/language.h>
#include <util/generic/hash_set.h>
#include <util/charset/wide.h>
#include <util/charset/unidata.h>

namespace NDecapitalizer {
    const TLangMask DEFAULT_LANG_MASK = TLangMask(LANG_ENG, LANG_RUS);
    const ELanguage DEFAULT_MAIN_LANGUAGE = LANG_RUS;

    void THintInfo::operator+=(const THintInfo& diff) {
        RegularCount += diff.RegularCount;
        CamelCount += diff.CamelCount;
        UpperCount += diff.UpperCount;
    }

    static void FillLemmas(const TLangMask& langs, const TWtringBuf& word, THashSet<TUtf16String>& lemmas) {
        TWLemmaArray larr;
        NLemmer::AnalyzeWord(word.data(), word.size(), larr, langs);
        for (const TYandexLemma& l : larr) {
            TUtf16String foundLemma(l.GetText(), l.GetTextLength());
            ToLower(foundLemma.begin(), foundLemma.size());
            lemmas.insert(foundLemma);
        }
    }

    TDecapitalizerHint::TDecapitalizerHint(const TLangMask& langs)
        : Langs(langs)
    {
    }

    void TDecapitalizerHint::AddWord(const TWtringBuf& word) {
        if (word.size() <= 1) {
            return;
        }
        size_t upperCnt = 0;
        for (wchar16 c : word) {
            if (IsUpper(c)) {
                ++upperCnt;
            }
        }
        THintInfo diff;
        if (upperCnt == 0 || (upperCnt == 1 && IsUpper(word[0]) && IsFirstWord)) {
            diff.RegularCount = 1;
        }
        if (upperCnt == 1 && IsUpper(word[0])) {
            diff.CamelCount = 1;
        }
        if (diff.RegularCount == 0 && diff.CamelCount == 0) {
            diff.UpperCount = 1;
        }
        THashSet<TUtf16String> lemmas;
        FillLemmas(Langs, word, lemmas);
        for (const TUtf16String& lemma : lemmas) {
            THintInfo info;
            if (Hints.find(lemma) != Hints.end()) {
                info = Hints[lemma];
            }
            info += diff;
            Hints[lemma] = info;
        }
    }

    void TDecapitalizerHint::OnToken(const TWideToken& token, size_t, NLP_TYPE type) {
        if (type == NLP_WORD) {
            for (const auto& subtoken : token.SubTokens) {
                if (subtoken.Type == TOKEN_WORD) {
                    AddWord(TUtf16String(token.Token + subtoken.Pos, subtoken.Len));
                    IsFirstWord = false;
                }
            }
        }
        if (type == NLP_SENTBREAK || type == NLP_PARABREAK) {
            IsFirstWord = true;
        }
    }

    THintInfo TDecapitalizerHint::GetInfo(const TWtringBuf& word) const {
        THashSet<TUtf16String> lemmas;
        FillLemmas(Langs, word, lemmas);
        for (const TUtf16String& lemma : lemmas) {
            const THintInfo* info = Hints.FindPtr(lemma);
            if (info != nullptr) {
                return *info;
            }
        }
        return THintInfo();
    }

    void TDecapitalizerHint::StartNewHint() {
        IsFirstWord = true;
    }

    static void FillHint(TDecapitalizerHint& hintHandler, const TVector<TUtf16String>& hints, const TTokenizerSplitParams& params) {
        TNlpTokenizer tokenizer(hintHandler, params.BackwardCompatibility);
        for (const auto& hint : hints) {
            hintHandler.StartNewHint();
            tokenizer.Tokenize(hint, params.SpacePreserve, params.TokenizerLangMask);
        }
    }

    TDecapitalizer::TDecapitalizer(const THashSet<TUtf16String>& abbrevs, const TVector<TUtf16String>& hints, ELanguage MainLanguage, const TLangMask& langs, const TTokenizerSplitParams& params)
        : Abbrevs(abbrevs)
        , Hint(langs)
        , Normalizer(NLemmer::GetAlphaRules(MainLanguage))
        , Params(params)
    {
        FillHint(Hint, hints, params);
    }

    void TDecapitalizer::OnToken(const TWideToken& token, size_t, NLP_TYPE type) {
        if (type == NLP_WORD) {
            for (const auto& subtoken : token.SubTokens) {
                if (subtoken.Type == TOKEN_WORD) {
                    TWtringBuf word(token.Token + subtoken.Pos, subtoken.Len);
                    PrvDecapitalized = DecapitalizeWord(word);
                    IsFirstWord = false;
                }
            }
        } else if (type == NLP_SENTBREAK || type == NLP_PARABREAK) {
            IsFirstWord = true;
        }
    }

    void TDecapitalizer::Decapitalize(TUtf16String& text) {
        Text = &text;
        IsFirstWord = true;
        PrvDecapitalized = false;
        TNlpTokenizer tokenizer(*this, Params.BackwardCompatibility);
        // text.begin() copies a string if it shared, so further we can easily modify string without taking care of loosing memory control
        tokenizer.Tokenize(text.begin(), text.size(), Params.SpacePreserve, Params.TokenizerLangMask);
    }

    bool TDecapitalizer::DecapitalizeWord(TWtringBuf& word) {
        if (word.size() == 1) {
            if (!PrvDecapitalized || IsFirstWord || !IsUpper(word[0])) {
                return false;
            }
            ToLower(word);
            return true;
        }
        size_t upperCnt = 0;
        size_t lowerCnt = 0;
        for (wchar16 c : word) {
            if (IsUpper(c) && !IsRomanDigit(c)) {
                ++upperCnt;
            }
            if (IsLower(c)) {
                ++lowerCnt;
            }
        }
        if (upperCnt <= 1 || lowerCnt > 0) {
            return false;
        }

        THintInfo hintInfo = Hint.GetInfo(word);
        if (hintInfo.RegularCount > 0 || hintInfo.CamelCount > 0 || hintInfo.UpperCount > 1) {
            return DecapitalizeByHint(word, hintInfo);
        }

        if (Abbrevs.find(word) != Abbrevs.end()) {
            return false;
        }
        ToLower(word, IsFirstWord);
        return true;
    }

    bool TDecapitalizer::DecapitalizeByHint(TWtringBuf& word, const THintInfo& hintInfo) {
        if (hintInfo.UpperCount > Max(hintInfo.RegularCount, hintInfo.CamelCount)) {
            return false;
        }
        bool isCamel = false;
        if (hintInfo.CamelCount > hintInfo.RegularCount) {
            isCamel = true;
        }
        ToLower(word, isCamel || IsFirstWord);
        return true;
    }

    void TDecapitalizer::ToLower(const TWtringBuf& word, bool skipFirst) {
        size_t offset = word.data() - Text->data();
        size_t len = word.size();
        if (skipFirst) {
            ++offset;
            --len;
        }
        TUtf16String substr = LegacySubstr(*Text, offset, len);
        Normalizer->ToLower(substr);
        if (substr.size() == len) {    // TODO: Can be false?
            Text->replace(offset, len, substr);
        }
    }
}
