#pragma once

#include <kernel/hitinfo/proto/skobkainfo.pb.h>

#include <util/generic/algorithm.h>

struct TSkobkaInfo {
    float DocFreq = 0;  // is normalized, i.e. DocFreq = CF / YandLen
    float DocFreqLnk = 0; // not used in denplusplus filtration
    ui64 ApproxHitCount = 1; // approximate number of matching positions in index <- not used in denplusplus filtration

    // Text and lemma should be wide enough to fit in UTF8-encoded words
    const static size_t MAX_TEXT = 64;
    char Text[MAX_TEXT];
    char Lemma[MAX_TEXT];
    bool LemmaIsBastard = true;
    ui8 LemmaQuality = 255;
    bool IsMultitoken = false;
    bool LemmaIsSubstantive = false;
    bool LemmaIsVerb = false; // not used in denplusplus filtration
    bool LemmaIsAdjective = false; // not used in denplusplus filtration
    bool LemmaIsGeo = false;
    bool IsGeoCity = false;
    bool IsGeoAddr = false;
    ui8 NumLemmas = 0; // not used in denplusplus filtration
    ui8 NumForms = 0; // not used in denplusplus filtration
    float WaresWeight = 0.f;

    TSkobkaInfo() {
        Text[0] = 0;
        Lemma[0] = 0;
    }

    void ToProto(NSkobkaInfo::TSkobkaInfo& info) const {
        info.SetDocFreq(DocFreq);
        info.SetDocFreqLnk(DocFreqLnk);
        info.SetApproxHitCount(ApproxHitCount);
        info.SetText(Text);
        info.SetLemma(Lemma);
        info.SetLemmaIsBastard(LemmaIsBastard);
        info.SetLemmaQuality(LemmaQuality);
        info.SetIsMultitoken(IsMultitoken);
        info.SetLemmaIsSubstantive(LemmaIsSubstantive);
        info.SetLemmaIsVerb(LemmaIsVerb);
        info.SetLemmaIsAdjective(LemmaIsAdjective);
        info.SetLemmaIsGeo(LemmaIsGeo);
        info.SetIsGeoCity(IsGeoCity);
        info.SetIsGeoAddr(IsGeoAddr);
        info.SetNumLemmas(NumLemmas);
        info.SetNumForms(NumForms);
        info.SetWaresWeight(WaresWeight);
    }

    void FromProto(const NSkobkaInfo::TSkobkaInfo& info) {
        DocFreq = info.GetDocFreq();
        DocFreqLnk = info.GetDocFreqLnk();
        ApproxHitCount = info.GetApproxHitCount();

        const ui32 textSize = Min<ui32>(MAX_TEXT - 1, info.GetText().size());
        Copy(info.GetText().begin(), info.GetText().begin() + textSize, Text);
        Text[textSize] = 0;

        const ui32 lemmaSize = Min<ui32>(MAX_TEXT - 1, info.GetLemma().size());
        Copy(info.GetLemma().begin(), info.GetLemma().begin() + lemmaSize, Lemma);
        Lemma[lemmaSize] = 0;

        LemmaIsBastard = info.GetLemmaIsBastard();
        LemmaQuality = info.GetLemmaQuality();
        IsMultitoken = info.GetIsMultitoken();
        LemmaIsSubstantive = info.GetLemmaIsSubstantive();
        LemmaIsVerb = info.GetLemmaIsVerb();
        LemmaIsAdjective = info.GetLemmaIsAdjective();
        LemmaIsGeo = info.GetLemmaIsGeo();
        IsGeoCity = info.GetIsGeoCity();
        IsGeoAddr = info.GetIsGeoAddr();
        NumLemmas = info.GetNumLemmas();
        NumForms = info.GetNumForms();
        WaresWeight = info.GetWaresWeight();
    }
};
