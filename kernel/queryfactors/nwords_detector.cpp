#include "nwords_detector.h"

void GetRelevZoneMultipliers(enum RelevLevel zone, float& multiplier1, float& multiplier2);

static void CalcResultsByZones(const ui32 position, const float weight, TAdultPreparat& adultPreparat) {
    float multFree = 0.0f;
    float multDepend = 0.0f;

    GetRelevZoneMultipliers(TWordPosition::GetRelevLevel(position), multFree, multDepend);

    adultPreparat.AdultFree += weight * multFree;
    adultPreparat.AdultDepend += weight * multDepend;
}

void TDetectorUnion::Prepare() {
    for(size_t i = 0; i < DetectorsList.size(); ++i)
        DetectorsList[i]->Prepare();
}

TDetectorUnion::TDetectorUnion(const NQueryFactors::TQueriesInfo& queries, TLemmasCollector& collector) {
    TDetector::TFactory detectorsFactory;
    for (size_t i = 0; i < queries.size(); ++i) {
        THolder<TDetector> detector(new TDetector);
        detector->Init(queries[i], collector, detectorsFactory);
        DetectorsList.push_back(detector.Release());
    }
}

bool TDetectorUnion::GetResult() {
    for (size_t i = 0; i < DetectorsList.size(); ++i) {
        if (DetectorsList[i]->GetResult())
            return true;
    }
    return false;
}

void TAdultDetector::T2CaseDetector::Detect(ui32 curWordPos, const TString& /*lemma*/) {
    if (!CurPositionIsCatched(curWordPos))
        return;

    // calc only if one word or began from second detectors
    if (PrevDetector || IsLast)
        CalcResultsByZones(curWordPos, Owner.GetWeight(), Owner.GetAdultPreparat());

    return;
}

TAdultDetector::TDetectorWithWeight::TDetectorWithWeight(float weight, TAdultPreparat& adultPreparat)
    : Weight(weight)
    , AdultPreparat(adultPreparat)
{ }

void TAdultDetector::Prepare() {
    AdultPreparat.Prepare();
    for (size_t i = 0; i < DetectorsStorage.size(); ++i)
        DetectorsStorage[i]->Prepare();
}

void TAdultDetector::AddWordsSequence2Detect(const TVector<TString>& lemmas, float weight, TLemmasCollector& lemmasCollector) {
    NQueryFactors::TQueryInfo queryPreparat;
    queryPreparat.reserve(10);

    for (size_t i = 0; i < lemmas.size(); ++i)
        queryPreparat.push_back(NQueryFactors::TQueryWordInfo(lemmas[i], /*distance =*/ 1));


    THolder<TDetectorWithWeight> detector(new TDetectorWithWeight(weight, AdultPreparat));
    TAdultDetector::T2CaseDetector::TFactory factory(*detector);
    detector->Init(queryPreparat, lemmasCollector, factory);
    DetectorsStorage.push_back(detector.Release());
}

