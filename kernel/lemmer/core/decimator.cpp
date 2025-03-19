#include "decimator.h"
#include "wordinstance.h"
#include <util/generic/hash_set.h>

const TFormDecimator TFormDecimator::EmptyDecimator;

using TWordSet = THashSet<TUtf16String>;

static inline bool CheckTypeDistance(const TGramBitSet& gram, const NSpike::TGrammarBunch& bunch) {
    for (const auto& it : bunch) {
        TGramBitSet diff = gram ^ it;
        if (diff.count() < 3 && !diff.Has(gDerivedAdjective))
            return true;
    }
    return false;
}

bool TFormDecimator::IsPreservedFeature(const TGramBitSet& feature, const NSpike::TGrammarBunch& goodFeatures, const NSpike::TGrammarBunch& exactFeatures) {
    return goodFeatures.find(feature) != goodFeatures.end() || CheckTypeDistance(feature, exactFeatures);
}

bool TFormDecimator::InitDecimator(IInputStream* input) {
    TermDecimator();
    try {
        ReadDataFile(*input);
    } catch (yexception exc) {
        LastError = exc.what();
        return false;
    }
    return true;
}

bool TFormDecimator::InitDecimator(const char* filename) {
    TIFStream input(filename);
    return InitDecimator(&input);
}

void TFormDecimator::ParseLine(const TUtf16String& s, ELanguage langcode, int /*version*/) {
    static TUtf16String max_forms_marker = u"MaxForms=";

    if (!s)
        return;

    size_t markerpos = s.find(max_forms_marker);
    if (markerpos != TUtf16String::npos) {
        TWtringBuf count = TWtringBuf(s).SubStr(markerpos + max_forms_marker.length());
        SetFormLimit(langcode, ::FromString(count));
        return;
    }

    AddGoodFeature(langcode, TGramBitSet::FromWtring(s));
}

#define GRAM_CHAR(x) char(x < 128 ? x : x - 256) // '(char)x' generates warning about overflow

bool TFormDecimator::DecimateLemmas(TWLemmaArray& lemmas) const {
    // Disambiguation, economy edition
    static const char strong_parts[] = {GRAM_CHAR(gParticle), GRAM_CHAR(gPostposition), GRAM_CHAR(gConjunction),
                                        GRAM_CHAR(gNumeral), GRAM_CHAR(gSubstPronoun), GRAM_CHAR(gAdvPronoun), 0};
    static const char nominal_parts[] = {GRAM_CHAR(gAdverb), GRAM_CHAR(gSubstantive), GRAM_CHAR(gAdjective),
                                         GRAM_CHAR(gAdjNumeral), GRAM_CHAR(gAdjPronoun), 0};
    static const char weak_nominal_features[] = {GRAM_CHAR(gPerson1), GRAM_CHAR(gPerson2),
                                                 GRAM_CHAR(gPoss1p), GRAM_CHAR(gPoss1pSg), GRAM_CHAR(gPoss1pPl),
                                                 GRAM_CHAR(gPoss2p), GRAM_CHAR(gPoss2pSg), GRAM_CHAR(gPoss2pPl),
                                                 GRAM_CHAR(gPredic1pSg), GRAM_CHAR(gPredic1pPl),
                                                 GRAM_CHAR(gPredic2pSg), GRAM_CHAR(gPredic2pPl),
                                                 0};
    TWordSet strongwords;
    TWordSet nominals;
    TWordSet weaknominals;
    TWordSet verbs;

    for (size_t i1 = 0; i1 < lemmas.size(); ++i1) {
        const TYandexLemma& lemma = lemmas[i1];
        if (lemma.GetLanguage() != LANG_TUR && lemma.GetLanguage() != LANG_KAZ)
            continue;
        const char* posptr = lemmas[i1].GetStemGram();
        if (!posptr || !*posptr)
            continue;

        TUtf16String text(lemma.GetText(), lemma.GetTextLength());
        if (strchr(strong_parts, *posptr))
            strongwords.insert(text);
        else if (GRAM_CHAR(gVerb) == *posptr)
            verbs.insert(text);
        else if (strchr(nominal_parts, *posptr)) {
            bool weak = lemma.FlexGramNum();
            const char* const* flexgrams = lemma.GetFlexGram();
            for (size_t i2 = 0; i2 < lemma.FlexGramNum() && weak; ++i2) {
                size_t weakpos = strcspn(weak_nominal_features, flexgrams[i2]);
                weak = weak_nominal_features[weakpos];
            }

            if (weak)
                weaknominals.insert(text);
            else
                nominals.insert(text);
        }
    }

    if (!strongwords.empty()) {
        // prefer function words to all other
        nominals.clear();
        verbs.clear();
    } else {
        if (nominals.empty() && !weaknominals.empty())
            nominals.swap(weaknominals);
        if (!nominals.empty()) {
            if (nominals.size() > 1) {
                // Exclude those that are 3 or less symbolss long, and fully contained in other
                TSet<TUtf16String> sortednominals(nominals.begin(), nominals.end());
                TSet<TUtf16String>::const_iterator it1, it2;
                for (it2 = sortednominals.begin(), it1 = it2++; it2 != sortednominals.end(); ++it2) {
                    if (it2->StartsWith(*it1) && it1->length() <= 3 && it1->length() < it2->length()) {
                        nominals.erase(*it1);
                    }
                    it1 = it2;
                }
            }

            TWordSet goodverbs;
            // Leave only verbs that coincide with verbal nouns
            for (auto maybeverb : nominals) {
                maybeverb.append((wchar16)'k'); // alma -> almak
                if (verbs.contains(maybeverb))
                    goodverbs.insert(maybeverb);
            }
            verbs.swap(goodverbs);
        }
    }

    bool touched = false;
    for (size_t i = lemmas.size(); i-- > 0;) {
        if (lemmas[i].GetLanguage() != LANG_TUR && lemmas[i].GetLanguage() != LANG_KAZ)
            continue;
        const char* posptr = lemmas[i].GetStemGram();
        if (!posptr || !*posptr)
            continue;

        const TUtf16String lemma(lemmas[i].GetText(), lemmas[i].GetTextLength());

        if (GRAM_CHAR(gVerb) == *posptr && !verbs.contains(lemma) || strchr(nominal_parts, *posptr) && !nominals.contains(lemma)) {
            // Cerr << "Erased lemma '" << WideToUTF8(lemma) << "' " << *posptr << Endl;
            lemmas.erase(lemmas.begin() + i);
            touched = true;
        }
    }
    Y_ASSERT(!lemmas.empty());

    return touched;
}

static inline TGramBitSet ExtractFormGrammems(const TYandexWordform& form, size_t flexIndex) {
    TGramBitSet gram;
    NSpike::ToGramBitset(form.GetStemGram(), form.GetFlexGram()[flexIndex], gram);
    return gram;
}

static inline void IncludeFormGrammems(const TYandexWordform& form, NSpike::TGrammarBunch& bunch) {
    NSpike::ToGrammarBunch(form.GetStemGram(), form.GetFlexGram(), form.FlexGramNum(), bunch);
}

template <typename T>
inline bool RemoveItems(TVector<T>& items, const TVector<size_t>& indices) {
    if (indices.empty())
        return false;

    size_t last = indices[0];
    for (size_t i = 1; i <= indices.size(); ++i) {
        size_t stop = (i < indices.size()) ? indices[i] : items.size();
        for (size_t idx = indices[i - 1] + 1; idx < stop; ++idx, ++last)
            items[last] = items[idx];
    }
    items.resize(last);
    return true;
}

bool TFormDecimator::DecimateForms(TLemmaForms& lemma) const {
    ELanguage lang = lemma.GetLanguage();
    if (!CheckLanguage(lang))
        return false;

    // Assessing the trouble
    TLemmaForms::TFormMap& forms = lemma.ObtainForms();
    if (!ExceedLimit(lang, forms.size()))
        return false;

    const NSpike::TGrammarBunch& goodfeatures = GoodFeaturesMap.find(lang)->second;

    // Filter rare forms, except the exact one and those close to it
    NSpike::TGrammarBunch exactfeatures;
    for (auto& form : forms) {
        TLemmaForms::TFormWeight& weight = form.second;
        if (weight.IsExact) {
            exactfeatures.insert(weight.Grams.begin(), weight.Grams.end());
        }
    }

    TWordSet badforms;
    for (auto& formit : forms) {
        const TUtf16String& form = formit.first;
        TLemmaForms::TFormWeight& weight = formit.second;

        bool preserved = false;
        for (NSpike::TGrammarBunch::const_iterator bunchit = weight.Grams.begin(); !preserved && bunchit != weight.Grams.end(); ++bunchit) {
            preserved = IsPreservedFeature(*bunchit, goodfeatures, exactfeatures);
        }
        if (!preserved)
            badforms.insert(form);
    }

    for (const auto& badform : badforms)
        forms.erase(badform);

    return !badforms.empty();
}

bool TFormDecimator::DecimateForms(const TYandexLemma& lemma, TWordformArray& forms) const {
    // same as previous for TYandexWordforms
    ELanguage lang = lemma.GetLanguage();
    if (!CheckLanguage(lang) || !ExceedLimit(lang, forms.size()))
        return false;

    const NSpike::TGrammarBunch& goodfeatures = GoodFeaturesMap.find(lang)->second;

    // Filter rare forms, except the exact one and those close to it
    NSpike::TGrammarBunch exactfeatures;
    TWtringBuf srcForm(lemma.GetNormalizedForm(), lemma.GetNormalizedFormLength());
    for (size_t i = 0; i < forms.size(); ++i)
        if (srcForm == forms[i].GetText())
            IncludeFormGrammems(forms[i], exactfeatures);

    TVector<size_t> badforms;
    for (size_t i = 0; i < forms.size(); ++i) {
        bool preserved = false;
        for (size_t f = 0; !preserved && f < forms[i].FlexGramNum(); ++f)
            preserved = IsPreservedFeature(ExtractFormGrammems(forms[i], f), goodfeatures, exactfeatures);

        if (!preserved)
            badforms.push_back(i);
    }

    return RemoveItems(forms, badforms);
}

TAutoPtr<IInputStream> OpenTDefaultDecimatorDataInputStream();

TDefaultFormDecimator::TDefaultFormDecimator() {
    TAutoPtr<IInputStream> data = OpenTDefaultDecimatorDataInputStream();
    InitDecimator(data.Get());
}
