#pragma once

#include <util/system/defaults.h>
#include <util/generic/vector.h>

#include "skobkainfo.h"

struct TTRIteratorHitInfo : public TSkobkaInfo {
    bool IsStopWord;
    float QuorumWordWeight;
    float WordWeight;
    float WordWeightLnk;

    float FiltrationWordWeight;
    float FiltrationWordLinkWeight;
    float FiltrationWordTextLinkWeight;

    TTRIteratorHitInfo() {
        // memset is important because of inheritance
        memset(this, 0, sizeof(*this));
    }

    void FillSkobkaInfo(const TSkobkaInfo& info) {
        DocFreq = info.DocFreq;
        DocFreqLnk = info.DocFreqLnk;
        ApproxHitCount = info.ApproxHitCount;
        memcpy(Text, info.Text, sizeof Text);
        memcpy(Lemma, info.Lemma, sizeof Lemma);
        LemmaIsBastard = info.LemmaIsBastard;
        LemmaQuality = info.LemmaQuality;
        IsMultitoken = info.IsMultitoken;
        LemmaIsSubstantive = info.LemmaIsSubstantive;
        LemmaIsVerb = info.LemmaIsVerb;
        LemmaIsAdjective = info.LemmaIsAdjective;
        LemmaIsGeo = info.LemmaIsGeo;
        IsGeoCity = info.IsGeoCity;
        IsGeoAddr = info.IsGeoAddr;
        NumLemmas = info.NumLemmas;
        NumForms = info.NumForms;
        WaresWeight = info.WaresWeight;
    }
};

using TTRIteratorHitInfos = TVector<TTRIteratorHitInfo>;

static inline void HitInfosToWordWeight(const TTRIteratorHitInfos& hitInfos, TVector<float>* weights) {
    weights->resize(hitInfos.size());
    for (size_t i = 0; i < hitInfos.size(); ++i) {
        (*weights)[i] = hitInfos[i].WordWeight;
    }
}
