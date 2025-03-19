#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/core/formgenerator.h>
#include <kernel/lemmer/dictlib/grambitset.h>
#include "declension.h"
#include "wtrutil.h"
////////////////////////////////////////////////////////////////////////////////////////////////////
static void GetWordVariants(const TUtf16String &word, EGrammar declension, TLangMask lang, TVector<TUtf16String> *res)
{
    TWLemmaArray lemmas;
    NLemmer::AnalyzeWord(word.data(), word.size(), lemmas, lang);
    for (size_t n = 0; n < lemmas.size(); ++n) {
        const TYandexLemma &lem = lemmas[n];
        for (size_t j = 0; j != lem.FlexGramNum(); ++j) {
            const char *grammar = lem.GetFlexGram()[j];
            if (NTGrammarProcessing::HasGram(grammar, gNominative)) {
                TVector<char> needed;
                needed.push_back(declension);
                for (const char *c = grammar; *c; ++c) {
                    EGrammar g = (EGrammar)((unsigned char)*c);
                    switch (g) {
                        case gFeminine:
                        case gMasculine:
                        case gNeuter:
                        case gSingular:
                        case gPlural:
                        case gPerfect:
                        case gImperfect:
                            needed.push_back(*c);
                            break;
                        default:
                            break;
                    }
                }
                needed.push_back(0);

                TWordformArray wordforms;
                NLemmer::Generate(lem, wordforms, &needed[0]);
                for (size_t k = 0; k < wordforms.size(); ++k) {
                    TUtf16String w = wordforms[k].GetText();
                    if (!Contains(*res, w))
                        res->push_back(w);
                }
            }
        }
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static void CombinatorialExplosion(TVector<TUtf16String> *res, const TVector<TUtf16String> &variants)
{
    size_t oldSize = res->size();
    if (variants.size() == 1 || (oldSize * variants.size()) > 32) {
        for (size_t m = 0; m < oldSize; ++m)
            (*res)[m] += variants[0];
    } else {
        TVector<TUtf16String> ret(variants.size() * oldSize);
        for (size_t n = 0; n < variants.size(); ++n)
            for (size_t m = 0; m < oldSize; ++m)
                ret[n * oldSize + m] = (*res)[m] + variants[n];
        std::swap(*res, ret);
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void PhraseToDeclension(TUtf16String phrase, EGrammar declension, TVector<TUtf16String> *res)
{
    TLangMask lang(LANG_RUS);

    TVector<TUtf16String> words;
    Wsplit(phrase.begin(), ' ', &words);
    TVector<TUtf16String> partialVariants;
    partialVariants.emplace_back();
    for (size_t n1 = 0; n1 < words.size(); ++n1) {
        if (n1) {
            for (size_t n2 = 0; n2 < partialVariants.size(); ++n2)
                partialVariants[n2] += ' ';
        }
        TVector<TUtf16String> variants;
        GetWordVariants(words[n1], declension, lang, &variants);
        if (variants.empty()) {
            for (size_t m = 0; m < partialVariants.size(); ++m)
                partialVariants[m] += words[n1];
        } else {
            if (!Contains(variants, words[n1]) && n1 > 0) {
                for (size_t m = 0; m < partialVariants.size(); ++m) {
                    res->push_back(partialVariants[m]);
                    res->back() += words[n1];
                    for (size_t k = n1 + 1; k < words.size(); ++k) {
                        res->back() += ' ';
                        res->back() += words[k];
                    }
                }
            }
            CombinatorialExplosion(&partialVariants, variants);
        }
    }
    for (size_t m = 0; m < partialVariants.size(); ++m)
        res->push_back(partialVariants[m]);
}

bool IsMatchGramPattern(const TUtf16String& phrase, const TGramBitSet& pattern) {
    if (pattern.none() || phrase.empty())
        return false;
    TLangMask lang(LANG_RUS);
    TWLemmaArray lemmas;
    NLemmer::AnalyzeWord(phrase.data(), phrase.size(), lemmas, lang);
    for (TWLemmaArray::iterator it = lemmas.begin(); it != lemmas.end(); ++it) {
        TGramBitSet stem = TGramBitSet::FromBytes(it->GetStemGram());
        if (it->FlexGramNum() == 0)
            return (pattern & stem) == pattern;
        for (size_t i = 0; i < it->FlexGramNum(); ++i) {
            TGramBitSet form = TGramBitSet::FromBytes(it->GetStemGram()) |
                TGramBitSet::FromBytes(it->GetFlexGram()[i]);
            if ((pattern & form) == pattern)
                return true;
        }
    }
    return false;
}
