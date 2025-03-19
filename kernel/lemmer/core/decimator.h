#pragma once

#include <util/generic/ptr.h>
#include <util/generic/map.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/singleton.h>
#include <library/cpp/langs/langs.h>
#include <library/cpp/wordlistreader/wordlistreader.h>
#include <util/string/vector.h>

#include <kernel/lemmer/dictlib/grambitset.h>

#include "formgenerator.h"
#include "lemmer.h"

class TLemmaForms;
class TFormDecimatorFilter;

class TFormDecimator: private TWordListReader {
public:
    TFormDecimator() {
    }
    ~TFormDecimator() override{};

    bool InitDecimator(const char* filename);
    bool InitDecimator(IInputStream* input);

    void TermDecimator() {
        GoodFeaturesMap.clear();
        LastError.clear();
    }

    bool DecimateLemmas(TWLemmaArray& lemmas) const;
    bool DecimateForms(TLemmaForms& lemma) const;
    bool DecimateForms(const TYandexLemma& lemma, TWordformArray& forms) const;

    static bool IsPreservedFeature(const TGramBitSet& feature, const NSpike::TGrammarBunch& goodFeatures, const NSpike::TGrammarBunch& exactFeatures);

    bool IsApplicable(const TYandexLemma& lemma) const {
        return CheckLanguage(lemma.GetLanguage()) && ExceedLimit(lemma.GetLanguage(), lemma.GetFormsCount());
    }

private:
    void ParseLine(const TUtf16String& line, ELanguage langcode, int version) override;

    void SetFormLimit(ELanguage lang, size_t formCount) {
        FormCountLimits[lang] = formCount;
    }

    void AddGoodFeature(ELanguage lang, const TGramBitSet& feature) {
        GoodFeaturesMap[lang].insert(feature);
    }

    bool CheckLanguage(ELanguage lang) const {
        if (lang != LANG_TUR && lang != LANG_KAZ)
            return false;
        if (!GoodFeaturesMap.contains(lang))
            return false;
        return true;
    }

    bool ExceedLimit(ELanguage lang, size_t count) const {
        TFormCountMap::const_iterator limit = FormCountLimits.find(lang);
        return limit == FormCountLimits.end() || count >= limit->second;
    }

private:
    friend class TFormDecimatorFilter;
    typedef THashMap<ELanguage, NSpike::TGrammarBunch> TGoodFeaturesMap;
    TGoodFeaturesMap GoodFeaturesMap;

    typedef THashMap<ELanguage, size_t> TFormCountMap;
    TFormCountMap FormCountLimits;

public:
    static const TFormDecimator EmptyDecimator;
    TString LastError; // last parsing error
};

// A decimator with builtin data
class TDefaultFormDecimator: public TFormDecimator {
public:
    TDefaultFormDecimator();
};

static inline const TFormDecimator& DefaultFormDecimator() {
    return Default<TDefaultFormDecimator>();
}

class TFormDecimatorFilter: public NLemmer::TGrammarFiltr {
public:
    TFormDecimatorFilter()
        : SlaveFilter(nullptr)
        , GoodFeatures(nullptr)
    {
    }

    TFormDecimatorFilter(const TFormDecimator* decimator, ELanguage lang, TAutoPtr<NLemmer::TGrammarFiltr> slaveFilter)
        : SlaveFilter(slaveFilter)
        , GoodFeatures(nullptr)
    {
        if (decimator) {
            TFormDecimator::TGoodFeaturesMap::const_iterator it = decimator->GoodFeaturesMap.find(lang);
            if (it != decimator->GoodFeaturesMap.end())
                GoodFeatures = &it->second;
        }
    }

    TFormDecimatorFilter(const TFormDecimatorFilter& other)
        : SlaveFilter(other.SlaveFilter ? other.SlaveFilter->Clone() : nullptr)
        , GoodFeatures(other.GoodFeatures)
        , ExactFeatures(other.ExactFeatures)
        , StemGram(other.StemGram)
    {
    }

    void SetLemma(const TYandexLemma& lemma) override {
        if (SlaveFilter)
            SlaveFilter->SetLemma(lemma);
        StemGram = TGramBitSet::FromBytes(lemma.GetStemGram());
        ExactFeatures.clear();
        if (lemma.FlexGramNum() > 0) {
            for (size_t i = 0; i != lemma.FlexGramNum(); ++i)
                ExactFeatures.insert(StemGram | TGramBitSet::FromBytes(lemma.GetFlexGram()[i]));
        } else {
            ExactFeatures.insert(StemGram);
        }
    }

    bool IsProperStemGr() const override {
        return !SlaveFilter || SlaveFilter->IsProperStemGr();
    }

    bool CheckFlexGr(const char* flexGram) const override {
        if (SlaveFilter && !SlaveFilter->CheckFlexGr(flexGram))
            return false;
        TGramBitSet feature = StemGram | TGramBitSet::FromBytes(flexGram);
        return TFormDecimator::IsPreservedFeature(feature, GoodFeatures ? *GoodFeatures : Default<NSpike::TGrammarBunch>(), ExactFeatures);
    }

    TFormDecimatorFilter* Clone() const override {
        return new TFormDecimatorFilter(*this);
    }

    ~TFormDecimatorFilter() override {
    }

private:
    THolder<NLemmer::TGrammarFiltr> SlaveFilter;
    const NSpike::TGrammarBunch* GoodFeatures;
    NSpike::TGrammarBunch ExactFeatures;
    TGramBitSet StemGram;
};
