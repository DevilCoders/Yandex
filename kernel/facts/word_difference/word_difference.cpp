#include <quality/trailer/trailer_common/normalize.h>
#include <kernel/lemmer/core/language.h>
#include <util/generic/hash_set.h>
#include "word_difference.h"
////////////////////////////////////////////////////////////////////////////////////////////////////
template <class T> void ToAllLemmas(const TVector<T> &words, THashSet<TUtf16String> &res) {
    for (TWtringBuf w : words) {
        TWLemmaArray lemmas;
        NLemmer::AnalyzeWord(w.data(), w.size(), lemmas, TLangMask(LANG_RUS, LANG_ENG));
        if (!lemmas.empty()) {
            for (const auto &lem : lemmas) {
                res.insert(TUtf16String(lem.GetText(), lem.GetTextLength()));
            }
        } else {
            res.insert(ToWtring(w));
        }
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void ToAllLemmas(TUtf16String q, THashSet<TUtf16String> &res) {
    TVector<const wchar16*> words;
    SplitIntoSuggestWords(q, &words);
    ToAllLemmas(words, res);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void ToExactWords(TUtf16String q, THashSet<TUtf16String> &res) {
    TVector<const wchar16*> words;
    SplitIntoSuggestWords(q, &words);
    for (TWtringBuf w : words)
        res.insert(ToWtring(w));
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void AsymmDifference(const THashSet<TUtf16String> &lem1, const THashSet<TUtf16String> &lem2, TUtf16String &res) {
    for (const auto &l1 : lem1) {
        if (!lem2.contains(l1)) {
            if (!res.empty())
                res.push_back(' ');
            res += l1;
        }
    }
    if (res.empty())
        res = u"__none__";
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CalcLemmaDifferences(TUtf16String q1, TUtf16String q2, TUtf16String &res1, TUtf16String &res2) {
    THashSet<TUtf16String> lem1, lem2;
    ToAllLemmas(q1, lem1);
    ToAllLemmas(q2, lem2);
    AsymmDifference(lem1, lem2, res1);
    AsymmDifference(lem2, lem1, res2);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CalcLemmaDifferences(const TVector<TUtf16String> &qWords1, const TVector<TUtf16String> &qWords2, TUtf16String &res1, TUtf16String &res2) {
    THashSet<TUtf16String> lem1, lem2;
    ToAllLemmas(qWords1, lem1);
    ToAllLemmas(qWords2, lem2);
    AsymmDifference(lem1, lem2, res1);
    AsymmDifference(lem2, lem1, res2);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CalcExactWordDifferences(TUtf16String q1, TUtf16String q2, TUtf16String &res1, TUtf16String &res2) {
    THashSet<TUtf16String> w1, w2;
    ToExactWords(q1, w1);
    ToExactWords(q2, w2);
    AsymmDifference(w1, w2, res1);
    AsymmDifference(w2, w1, res2);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CalcExactWordDifferences(const TVector<TUtf16String> &qWords1, const TVector<TUtf16String> &qWords2, TUtf16String &res1, TUtf16String &res2) {
    THashSet<TUtf16String> w1(qWords1.begin(), qWords1.end()), w2(qWords2.begin(), qWords2.end());
    AsymmDifference(w1, w2, res1);
    AsymmDifference(w2, w1, res2);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
