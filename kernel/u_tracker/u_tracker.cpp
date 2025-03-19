#include "u_tracker.h"

#include <kernel/factor_storage/float_utils.h>

#include <util/generic/utility.h>
#include <util/generic/ymath.h>
#include <util/string/cast.h>
#include <util/stream/output.h>

namespace {
    const float FloatEpsilon = 1e-6f;

    inline float UFeature(float value) {
        return SoftClipFloatTo01(value, FloatEpsilon);
    }
}

template<ui8 MaxCalcLevel>
void TUTrackerWrapperWithLevel<MaxCalcLevel>::AddHit(ui32 leftBreak, ui32 breakLength, const TFullPosition& pos, const size_t wordIdx, ui32 data)
{
    if (Y_UNLIKELY(UTracker.Get() == nullptr)) {
        return;
    }

    if (LastBreak != leftBreak) {
        if (LastBreak != InvalidBreak) {
            UTracker->FinishSent<MaxCalcLevel>();
        }
        const float sentWeight = GetWeight(data);

        UTracker->StartSent<MaxCalcLevel>(sentWeight, breakLength);
        LastBreak = leftBreak;
    }
    UTracker->AddHit<MaxCalcLevel>(pos, wordIdx);
}


template<ui8 MaxCalcLevel>
void TUTrackerWrapperWithLevel<MaxCalcLevel>::AddSentence(
    ui32 leftBreak,
    ui32 breakLength,
    const TFullPositionEx* posBegin,
    const TFullPositionEx* posEnd,
    ui32 data)
{
    if (Y_UNLIKELY(UTracker.Get() == nullptr)) {
        return;
    }

    if (LastBreak != InvalidBreak) {
        UTracker->FinishSent<MaxCalcLevel>();
    }
    const float sentWeight = GetWeight(data);

    UTracker->StartSent<MaxCalcLevel>(sentWeight, breakLength);
    LastBreak = leftBreak;

    for (; posBegin != posEnd; ++posBegin) {
        UTracker->AddHit<MaxCalcLevel>(posBegin->Pos, posBegin->WordIdx);
    }
}

template<ui8 MaxCalcLevel>
const TUTracker* TUTrackerWrapperWithLevel<MaxCalcLevel>::FinishDoc()
{
    if (Y_UNLIKELY(UTracker.Get() == nullptr)) {
        return nullptr;
    }

    if (LastBreak != InvalidBreak) {
        UTracker->FinishSent<MaxCalcLevel>();
        LastBreak = InvalidBreak;
    }

    Y_IF_DEBUG(
                Y_VERIFY(MaxCalcLevel == UTracker->DeclaredCalcLevel, "wrong class usage: using variable MaxCalcLevel is forbidden");
                if (UTracker->MaxCalledCalcLevel && UTracker->MaxCalledCalcLevel < UTracker->DeclaredCalcLevel)
                    Cerr << "Hint: you can boost performance by using TUTrackerWrapperWithLevel with MaxCalcLevel=" << ToString(UTracker->MaxCalledCalcLevel) << ", actually used: " << ToString(UTracker->DeclaredCalcLevel) << Endl;
                )

    return UTracker.Get();
}

template<ui8 MaxCalcLevel>
void TUTrackerWrapperWithLevel<MaxCalcLevel>::InitNextDoc()
{
    if (Y_UNLIKELY(UTracker.Get() == nullptr)) {
        return;
    }

    UTracker->NewDoc<MaxCalcLevel>();
    LastBreak = InvalidBreak;
}

template class TUTrackerWrapperWithLevel<0>;
template class TUTrackerWrapperWithLevel<1>;
template class TUTrackerWrapperWithLevel<2>;
template class TUTrackerWrapperWithLevel<3>;

TUTracker::TPrecalculatedTable::TPrecalculatedTable() {
    for (ui32 i = 0; i < MaxWords; ++i) {
        Distance[i] = 1.f / float((i + 1) * (i + 1));
    }
    for (ui32 i = 0; i < MaxWords; ++i) {
        Deviation[i] = 1.f / powf(float(i + 1), 1.5f);
    }
    for (ui32 i = 0; i < MaxWords; ++i) {
        Attenuation[i] = 1.f / float(i + 1);
    }
    for (ui32 i = 0; i < 1024; ++i) {
        GlobalAttenuation[i] = 10.f / float(i + 10);
    }
}

const TUTracker::TPrecalculatedTable TUTracker::PrecalculatedTable;

TUTracker::TUTracker(const TVector<float>& wordWeight)
    : QueryWordCount(Min<size_t>(wordWeight.size(), MaxWords))
{
    Copy(wordWeight.begin(), wordWeight.begin() + QueryWordCount, WordAbsWeight);

    float sum = 0.000001f;
    float sum2 = 0.000001f;
    for (auto i = 0u; i < QueryWordCount; ++i) {
        sum += WordAbsWeight[i] + 0.0001f;
        sum2 += WordAbsWeight[i] * WordAbsWeight[i] + 0.0001f;
    }
    for (auto i = 0u; i < QueryWordCount; ++i) {
        WordRelWeight[i] = (WordAbsWeight[i] + 0.0001f) / sum;
        WordRel2Weight[i] = (WordAbsWeight[i] * WordAbsWeight[i] + 0.0001f) / sum2;
    }
}


float TUTracker::CalcFormWeightSoft(const TFullPosition& pos) const noexcept {
    return TWordPosition::Form(pos.End) <= EQUAL_BY_STRING ? 1.f : 0.9f;
}


float TUTracker::CalcFormWeightStrict(const TFullPosition& pos) const noexcept {
    return TWordPosition::Form(pos.End) <= EQUAL_BY_STRING ? 1.f : 0.01f;
}


float TUTracker::CalcProximity(const ui32 leftPosition, const ui32 rightPosition) const noexcept {
    return leftPosition >= rightPosition ? 1 : PrecalculatedTable.Distance[rightPosition - leftPosition - 1];
}


float TUTracker::CalcDeviation(const ui32 queryPosition, const ui32 hitPosition) const noexcept {
    return queryPosition == hitPosition ? 1 : PrecalculatedTable.Deviation[
                                          queryPosition > hitPosition ?
                queryPosition - hitPosition :
                hitPosition - queryPosition];
}


float TUTracker::CalcAttenuation(const ui32 position) const noexcept {
    return PrecalculatedTable.Attenuation[position];
}


float TUTracker::CalcGlobalAttenuation(const ui32 position) const noexcept {
    return PrecalculatedTable.GlobalAttenuation[Min(position, 1023u)];
}


ui32 TUTracker::GetLeftPosition(const TFullPosition& pos) const noexcept {
    return TWordPosition::Word(pos.Beg) - 1;
}


ui32 TUTracker::GetRightPosition(const TFullPosition& pos) const noexcept {
    return TWordPosition::Word(pos.End) - 1;
}


float TUTracker::CalcMatchMix(const float* acc1, const float* acc2, const float alpha, const float k) const noexcept { // requires level 0
    float result = 0;
    for (auto i = 0u; i < QueryWordCount; ++i) {
        result += CalcTf((1.f - alpha) * acc1[i] + alpha * acc2[i], k) * WordRelWeight[i];
    }
    return result;
}


float TUTracker::CalcMatchMixW1(const float* acc1, const float* acc2, const float alpha, const float k) const noexcept { // requires level 0
    float result = 0;
    for (auto i = 0u; i < QueryWordCount; ++i) {
        const float frq = (1.f - alpha) * acc1[i] + alpha * acc2[i];
        const float tf = CalcTf(frq, k);
        result += tf;
    }
    return result / float(QueryWordCount);
}


float TUTracker::CalcTf(const float value, const float k) const noexcept {
    return value / (value + k);
}

float TUTracker::CalcMatch(const float* acc, const float k) const noexcept { // requires level 0
    float result = 0;
    for (auto i = 0u; i < QueryWordCount; ++i) {
        result += CalcTf(acc[i], k) * WordRelWeight[i];
    }
    return result;
}

float TUTracker::CalcMatchW1(const float* acc, const float k) const noexcept { // requires level 0
    float result = 0;
    for (auto i = 0u; i < QueryWordCount; ++i) {
        result += CalcTf(acc[i], k);
    }
    return result / (float)QueryWordCount;
}


float TUTracker::CalcMatchW2(const float* acc, const float k) const noexcept { // requires level 2
    float result = 0;
    for (auto i = 0u; i < QueryWordCount; ++i) {
        result += CalcTf(acc[i], k) * WordRel2Weight[i];
    }
    return result;
}


float TUTracker::CalcMatchMax(const float* acc, const float k) const noexcept { // requires level 0
    if (QueryWordCount == 1) {
        return CalcTf(acc[0], k);
    }
    float maxWordWeight = WordRelWeight[0];
    int maxIdx = 0;
    for (auto i = 1u; i < QueryWordCount; ++i) {
        if (WordRelWeight[i] > maxWordWeight) {
            maxWordWeight = WordRelWeight[i];
            maxIdx = i;
        }
    }
    return CalcTf(acc[maxIdx], k);
}


float TUTracker::CalcBclm(const float* acc, const float k) const noexcept { // requires level 0
    return CalcMatch(QueryWordCount == 1 ? WordAcc : acc, k);
}


float TUTracker::CalcBclmW1(const float* acc, const float k) const noexcept { // requires level 0
    return CalcMatchW1(QueryWordCount == 1 ? WordAcc : acc, k);
}


float TUTracker::CalcBclmW2(const float* acc, const float k) const noexcept { // requires level 2
    return CalcMatchW2(QueryWordCount == 1 ? WordAcc : acc, k);
}


float TUTracker::CalcFeatureImpl(EFeature feature, float k, float alpha) const
{
    switch (feature) {
        case FBm15:
            return CalcBm15(k);
        case FBm15W1:
            return CalcBm15W1(k);
        case FBm15W2:
            return CalcBm15W2(k);
        case FBm15V0:
            return CalcBm15V0(k);
        case FBm15V0W1:
            return CalcBm15V0W1(k);
        case FBm15V0W2:
            return CalcBm15V0W2(k);
        case FBm15V2:
            return CalcBm15V2(k);
        case FBm15V2W1:
            return CalcBm15V2W1(k);
        case FBm15V2W2:
            return CalcBm15V2W2(k);
        case FBm15V4:
            return CalcBm15V4(k);
        case FBm15V4W1:
            return CalcBm15V4W1(k);
        case FBm15V4W2:
            return CalcBm15V4W2(k);
        case FBm15VA:
            return CalcBm15VA(k);
        case FBm15VAW1:
            return CalcBm15VAW1(k);
        case FBm15VAW2:
            return CalcBm15VAW2(k);
        case FBm15Max:
            return CalcBm15Max(k);
        case FBm15V2Max:
            return CalcBm15V2Max(k);
        case FBm15Strict:
            return CalcBm15Strict(k);
        case FBm15StrictW1:
            return CalcBm15StrictW1(k);
        case FBm15StrictW2:
            return CalcBm15StrictW2(k);
        case FBm15Atten:
            return CalcBm15Atten(k);
        case FBm15AttenW1:
            return CalcBm15AttenW1(k);
        case FBm15AttenW2:
            return CalcBm15AttenW2(k);
        case FBm15Wcm:
            return CalcBm15Wcm(k);
        case FBm15WcmW1:
            return CalcBm15WcmW1(k);
        case FBm15WcmW2:
            return CalcBm15WcmW2(k);
        case FBm15Coverage:
            return CalcBm15Coverage(k);
        case FBm15CoverageW1:
            return CalcBm15CoverageW1(k);
        case FBm15CoverageV2:
            return CalcBm15CoverageV2(k);
        case FBm15CoverageV2W1:
            return CalcBm15CoverageV2W1(k);
        case FBm15CoverageV4:
            return CalcBm15CoverageV4(k);
        case FBm15CoverageV4W1:
            return CalcBm15CoverageV4W1(k);
        case FBclmPlain:
            return CalcBclmPlain(k);
        case FBclmPlainW1:
            return CalcBclmPlainW1(k);
        case FBclmPlainW2:
            return CalcBclmPlainW2(k);
        case FBclmPlainV2:
            return CalcBclmPlainV2(k);
        case FBclmPlainV2W1:
            return CalcBclmPlainV2W1(k);
        case FBclmPlainV2W2:
            return CalcBclmPlainV2W2(k);
        case FBclmMixPlain:
            return CalcBclmMixPlain(k, alpha);
        case FBclmMixPlainW1:
            return CalcBclmMixPlainW1(k, alpha);
        case FBclmMixPlainV2:
            return CalcBclmMixPlainV2(k, alpha);
        case FBclmMixPlainV2W1:
            return CalcBclmMixPlainV2W1(k, alpha);
        case FBclmWeighted:
            return CalcBclmWeighted(k);
        case FBclmMixWeighted:
            return CalcBclmMixWeighted(k, alpha);
        case FBclmWeightedW1:
            return CalcBclmWeightedW1(k);
        case FBclmWeightedW2:
            return CalcBclmWeightedW2(k);
        case FBclmWeightedV2:
            return CalcBclmWeightedV2(k);
        case FBclmWeightedV2W1:
            return CalcBclmWeightedV2W1(k);
        case FBclmWeightedV2W2:
            return CalcBclmWeightedV2W2(k);
        case FBclmSoft:
            return CalcBclmSoft(k);
        case FBclmHard:
            return CalcBclmHard(k);
        case FBclmHardW1:
            return CalcBclmHardW1(k);
        case FBclmHardW2:
            return CalcBclmHardW2(k);
        case FBocmPlain:
            return CalcBocmPlain(k);
        case FBocmWeighted:
            return CalcBocmWeighted(k);
        case FBocmWeightedW1:
            return CalcBocmWeightedW1(k);
        case FBocmWeightedW2:
            return CalcBocmWeightedW2(k);
        case FBocmWeightedV2:
            return CalcBocmWeightedV2(k);
        case FBocmWeightedV2W1:
            return CalcBocmWeightedV2W1(k);
        case FBocmWeightedV2W2:
            return CalcBocmWeightedV2W2(k);
        case FBocmWeightedV4:
            return CalcBocmWeightedV4(k);
        case FBocmWeightedV4W1:
            return CalcBocmWeightedV4W1(k);
        case FBocmWeightedV4W2:
            return CalcBocmWeightedV4W2(k);
        case FBocmWeightedMax:
            return CalcBocmWeightedMax(k);
        case FBocmWeightedV2Max:
            return CalcBocmWeightedV2Max(k);
        case FBocmDouble:
            return CalcBocmDouble(k);
        case FBocmDoubleW1:
            return CalcBocmDoubleW1(k);
        case FBocmDoubleW2:
            return CalcBocmDoubleW2(k);
        case FBocmDoubleV2:
            return CalcBocmDoubleV2(k);
        case FBocmDoubleV2W1:
            return CalcBocmDoubleV2W1(k);
        case FBocmDoubleV2W2:
            return CalcBocmDoubleV2W2(k);
        case FBocmDoubleMax:
            return CalcBocmDoubleMax(k);
        case FBocmDoubleV2Max:
            return CalcBocmDoubleV2Max(k);
        case FFullMatchPrediction:
            return CalcFullMatchPrediction();
        case FSynonymMatchPrediction:
            return CalcSynonymMatchPrediction();
        case FAnnotationMatchPrediction:
            return CalcAnnotationMatchPrediction();
        case FQueryMatchPrediction:
            return CalcQueryMatchPrediction();
        case FAnnotationMatchPredictionWeighted:
            return CalcAnnotationMatchPredictionWeighted();

        default:
            Y_ASSERT(false && "wrong feature index");
            return 0;
    }
}

float TUTracker::CalcFeature(EFeature feature, float k, float alpha) const {
    return CalcFeatureImpl(feature, k, alpha);
}

#ifndef NDEBUG
#define FEATURE_LEVEL(LEVEL) \
    Y_ASSERT((MaxCalledCalcLevel = Max<ui8>(LEVEL, MaxCalledCalcLevel)) <= DeclaredCalcLevel && "too low MaxCalcLevel declared, need at least " #LEVEL);
#else
#define FEATURE_LEVEL(LEVEL) Y_ASSERT(true);
#endif

float TUTracker::CalcValueWcmMax() const {
    FEATURE_LEVEL(0);
    return UFeature(ValueWcmMax);
}


float TUTracker::CalcWcmMax() const {
    FEATURE_LEVEL(0);
    return UFeature(WcmMax);
}


float TUTracker::CalcValueWcmPrediction() const {
    FEATURE_LEVEL(0);
    return UFeature(ValueWcmMax * WcmMax);
}


float TUTracker::CalcValueWcmAvg() const {
    FEATURE_LEVEL(0);
    return WcmSum <= 0 ? 0 : UFeature(ValueWcmSum / WcmSum);
}


float TUTracker::CalcValuePcmMax() const {
    FEATURE_LEVEL(2);
    return UFeature(ValuePcmMax);
}


float TUTracker::CalcPcmMax() const {
    FEATURE_LEVEL(2);
    return UFeature(PcmMax);
}


float TUTracker::CalcValuePcmPrediction() const {
    FEATURE_LEVEL(2);
    return UFeature(ValuePcmMax * PcmMax);
}


float TUTracker::CalcValuePcmAvg() const {
    FEATURE_LEVEL(2);
    return PcmSum <= 0 ? 0 : UFeature(ValuePcmSum / PcmSum);
}


float TUTracker::CalcFullMatchPrediction() const {
    FEATURE_LEVEL(0);
    return UFeature(FullMatchValue);
}


float TUTracker::CalcSynonymMatchPrediction() const {
    FEATURE_LEVEL(0);
    return UFeature(SynonymMatchValue);
}


float TUTracker::CalcAnnotationMatchPrediction() const {
    FEATURE_LEVEL(0);
    return UFeature(AnnotationMatchValue);
}

float TUTracker::CalcAnnotationMatchPredictionWeighted() const {
    FEATURE_LEVEL(0);
    return UFeature(AnnotationMatchWeightedValue);
}

float TUTracker::CalcQueryMatchPrediction() const {
    FEATURE_LEVEL(0);
    return UFeature(QueryMatchValue);
}


float TUTracker::CalcWcmCoveragePrediction() const {
    return UFeature(ValueWcmCoverageMax);
}


float TUTracker::CalcWcmCoverageMax() const {
    return UFeature(WcmCoverageMax);
}


float TUTracker::CalcPrefixMatchMax() const {
    return UFeature(PrefixMatchMax);
}


float TUTracker::CalcPrefixMatchAvg() const {
    return PrefixMatchCount == 0 ? 0 : UFeature(PrefixMatchSum / float(PrefixMatchCount));
}


float TUTracker::CalcPrefixMatchCount() const {
    return UFeature(float(PrefixMatchCount) / float(PrefixMatchCount + 10));
}


float TUTracker::CalcSuffixMatchMax() const {
    return UFeature(SuffixMatchMax);
}


float TUTracker::CalcSuffixMatchAvg() const {
    return SuffixMatchCount == 0 ? 0 : UFeature(SuffixMatchSum / float(SuffixMatchCount));
}


float TUTracker::CalcSuffixMatchCount() const {
    return UFeature(float(SuffixMatchCount) / float(SuffixMatchCount + 10));
}



float TUTracker::CalcBm15(const float k) const {
    FEATURE_LEVEL(0);
    return UFeature(CalcMatch(WordAcc, k));
}


float TUTracker::CalcBm15W1(const float k) const {
    FEATURE_LEVEL(0);
    return UFeature(CalcMatchW1(WordAcc, k));
}


float TUTracker::CalcBm15W2(const float k) const {
    FEATURE_LEVEL(2);
    return UFeature(CalcMatchW2(WordAcc, k));
}


float TUTracker::CalcBm15V0(const float k) const {
    FEATURE_LEVEL(2);
    return UFeature(CalcMatch(WordV0Acc, k));
}


float TUTracker::CalcBm15V0W1(const float k) const {
    FEATURE_LEVEL(2);
    return UFeature(CalcMatchW1(WordV0Acc, k));
}


float TUTracker::CalcBm15V0W2(const float k) const {
    FEATURE_LEVEL(2);
    return UFeature(CalcMatchW2(WordV0Acc, k));
}


float TUTracker::CalcBm15V2(const float k) const {
    FEATURE_LEVEL(1);
    return UFeature(CalcMatch(WordV2Acc, k));
}


float TUTracker::CalcBm15V2W1(const float k) const {
    FEATURE_LEVEL(1);
    return UFeature(CalcMatchW1(WordV2Acc, k));
}


float TUTracker::CalcBm15V2W2(const float k) const {
    FEATURE_LEVEL(1);
    return UFeature(CalcMatchW2(WordV2Acc, k));
}


float TUTracker::CalcBm15V4(const float k) const {
    FEATURE_LEVEL(1);
    return UFeature(CalcMatch(WordV4Acc, k));
}


float TUTracker::CalcBm15V4W1(const float k) const {
    FEATURE_LEVEL(1);
    return UFeature(CalcMatchW1(WordV4Acc, k));
}


float TUTracker::CalcBm15V4W2(const float k) const {
    FEATURE_LEVEL(2);
    return UFeature(CalcMatchW2(WordV4Acc, k));
}


float TUTracker::CalcBm15VA(const float k) const {
    FEATURE_LEVEL(3);
    return UFeature(CalcMatch(WordVAAcc, k));
}


float TUTracker::CalcBm15VAW1(const float k) const {
    FEATURE_LEVEL(3);
    return UFeature(CalcMatchW1(WordVAAcc, k));
}


float TUTracker::CalcBm15VAW2(const float k) const {
    FEATURE_LEVEL(2);
    return UFeature(CalcMatchW2(WordVAAcc, k));
}


float TUTracker::CalcBm15Strict(const float k) const {
    FEATURE_LEVEL(0);
    return UFeature(CalcMatch(WordStrictAcc, k));
}


float TUTracker::CalcBm15StrictW1(const float k) const {
    FEATURE_LEVEL(0);
    return UFeature(CalcMatchW1(WordStrictAcc, k));
}


float TUTracker::CalcBm15StrictW2(const float k) const {
    FEATURE_LEVEL(2);
    return UFeature(CalcMatchW2(WordStrictAcc, k));
}


float TUTracker::CalcBm15Max(const float k) const {
    FEATURE_LEVEL(0);
    return UFeature(CalcMatchMax(WordAcc, k));
}


float TUTracker::CalcBm15V2Max(const float k) const {
    FEATURE_LEVEL(1);
    return UFeature(CalcMatchMax(WordV2Acc, k));
}


float TUTracker::CalcBm15Atten(const float k) const {
    FEATURE_LEVEL(2);
    return UFeature(CalcMatch(WordAttenAcc, k));
}


float TUTracker::CalcBm15AttenW1(const float k) const {
    FEATURE_LEVEL(2);
    return UFeature(CalcMatchW1(WordAttenAcc, k));
}


float TUTracker::CalcBm15AttenW2(const float k) const {
    FEATURE_LEVEL(2);
    return UFeature(CalcMatchW2(WordAttenAcc, k));
}


float TUTracker::CalcBm15Wcm(const float k) const {
    FEATURE_LEVEL(1);
    return UFeature(CalcMatch(WordWcmAcc, k));
}


float TUTracker::CalcBm15WcmW1(const float k) const {
    FEATURE_LEVEL(1);
    return UFeature(CalcMatchW1(WordWcmAcc, k));
}


float TUTracker::CalcBm15WcmW2(const float k) const {
    FEATURE_LEVEL(2);
    return UFeature(CalcMatchW2(WordWcmAcc, k));
}


float TUTracker::CalcBm15Coverage(const float k) const {
    FEATURE_LEVEL(1);
    return UFeature(CalcMatch(WordCoverageAcc, k));
}


float TUTracker::CalcBm15CoverageW1(const float k) const {
    FEATURE_LEVEL(1);
    return UFeature(CalcMatchW1(WordCoverageAcc, k));
}


float TUTracker::CalcBm15CoverageV2(const float k) const {
    FEATURE_LEVEL(1);
    return UFeature(CalcMatch(WordCoverageV2Acc, k));
}


float TUTracker::CalcBm15CoverageV2W1(const float k) const {
    FEATURE_LEVEL(1);
    return UFeature(CalcMatchW1(WordCoverageV2Acc, k));
}


float TUTracker::CalcBm15CoverageV4(const float k) const {
    FEATURE_LEVEL(1);
    return UFeature(CalcMatch(WordCoverageV4Acc, k));
}


float TUTracker::CalcBm15CoverageV4W1(const float k) const {
    FEATURE_LEVEL(1);
    return UFeature(CalcMatchW1(WordCoverageV4Acc, k));
}

float TUTracker::CalcBclmPlain(const float k) const {
    FEATURE_LEVEL(0);
    return UFeature(CalcBclm(WordBclmPlainAcc, k));
}


float TUTracker::CalcBclmPlainW1(const float k) const {
    FEATURE_LEVEL(0);
    return UFeature(CalcBclmW1(WordBclmPlainAcc, k));
}


float TUTracker::CalcBclmPlainW2(const float k) const {
    FEATURE_LEVEL(2);
    return UFeature(CalcBclmW2(WordBclmPlainAcc, k));
}


float TUTracker::CalcBclmPlainV2(const float k) const {
    FEATURE_LEVEL(1);
    return UFeature(CalcBclm(WordBclmPlainV2Acc, k));
}


float TUTracker::CalcBclmPlainV2W1(const float k) const {
    FEATURE_LEVEL(1);
    return UFeature(CalcBclmW1(WordBclmPlainV2Acc, k));
}


float TUTracker::CalcBclmPlainV2W2(const float k) const {
    FEATURE_LEVEL(2);
    return UFeature(CalcBclmW2(WordBclmPlainV2Acc, k));
}


float TUTracker::CalcBclmMixPlain(const float k, const float alpha) const {
    FEATURE_LEVEL(0);
    return UFeature(CalcMatchMix(WordBclmPlainAcc, WordAcc, alpha, k));
}


float TUTracker::CalcBclmMixPlainW1(const float k, const float alpha) const {
    FEATURE_LEVEL(0);
    return UFeature(CalcMatchMixW1(WordBclmPlainAcc, WordAcc, alpha, k));
}


float TUTracker::CalcBclmMixPlainV2(const float k, const float alpha) const {
    FEATURE_LEVEL(1);
    return UFeature(CalcMatchMix(WordBclmPlainV2Acc, WordV2Acc, alpha, k));
}


float TUTracker::CalcBclmMixPlainV2W1(const float k, const float alpha) const {
    FEATURE_LEVEL(1);
    return UFeature(CalcMatchMixW1(WordBclmPlainV2Acc, WordV2Acc, alpha, k));
}


float TUTracker::CalcBclmWeighted(const float k) const {
    FEATURE_LEVEL(0);
    return UFeature(CalcBclm(WordBclmWeightedAcc, k));
}


float TUTracker::CalcBclmWeightedW1(const float k) const {
    FEATURE_LEVEL(0);
    return UFeature(CalcBclmW1(WordBclmWeightedAcc, k));
}


float TUTracker::CalcBclmWeightedW2(const float k) const {
    FEATURE_LEVEL(2);
    return UFeature(CalcBclmW2(WordBclmWeightedAcc, k));
}


float TUTracker::CalcBclmMixWeighted(const float k, const float alpha) const {
    FEATURE_LEVEL(0);
    return UFeature(CalcMatchMix(WordBclmWeightedAcc, WordAcc, alpha, k));
}


float TUTracker::CalcBclmWeightedV2(const float k) const {
    FEATURE_LEVEL(1);
    return UFeature(CalcBclm(WordBclmWeightedV2Acc, k));
}


float TUTracker::CalcBclmWeightedV2W1(const float k) const {
    FEATURE_LEVEL(2);
    return UFeature(CalcBclmW1(WordBclmWeightedV2Acc, k));
}


float TUTracker::CalcBclmWeightedV2W2(const float k) const {
    FEATURE_LEVEL(2);
    return UFeature(CalcBclmW2(WordBclmWeightedV2Acc, k));
}


float TUTracker::CalcBclmSoft(const float k) const {
    FEATURE_LEVEL(2);
    return UFeature(CalcBclm(WordBclmSoftAcc, k));
}


float TUTracker::CalcBclmHard(const float k) const {
    FEATURE_LEVEL(2);
    return UFeature(CalcBclm(WordBclmHardAcc, k));
}


float TUTracker::CalcBclmHardW1(const float k) const {
    FEATURE_LEVEL(2);
    return UFeature(CalcBclmW1(WordBclmHardAcc, k));
}


float TUTracker::CalcBclmHardW2(const float k) const {
    FEATURE_LEVEL(2);
    return UFeature(CalcBclmW2(WordBclmHardAcc, k));
}


float TUTracker::CalcBocmPlain(const float k) const {
    FEATURE_LEVEL(1);
    return UFeature(CalcMatch(WordBocmPlainAcc, k));
}


float TUTracker::CalcBocmWeighted(const float k) const {
    FEATURE_LEVEL(0);
    return UFeature(CalcMatch(WordBocmWeightedAcc, k));
}


float TUTracker::CalcBocmWeightedW1(const float k) const {
    FEATURE_LEVEL(0);
    return UFeature(CalcMatchW1(WordBocmWeightedAcc, k));
}


float TUTracker::CalcBocmWeightedW2(const float k) const {
    FEATURE_LEVEL(2);
    return UFeature(CalcMatchW2(WordBocmWeightedAcc, k));
}


float TUTracker::CalcBocmWeightedV2(const float k) const {
    FEATURE_LEVEL(1);
    return UFeature(CalcMatch(WordBocmWeightedV2Acc, k));
}


float TUTracker::CalcBocmWeightedV2W1(const float k) const {
    FEATURE_LEVEL(1);
    return UFeature(CalcMatchW1(WordBocmWeightedV2Acc, k));
}


float TUTracker::CalcBocmWeightedV2W2(const float k) const {
    FEATURE_LEVEL(2);
    return UFeature(CalcMatchW2(WordBocmWeightedV2Acc, k));
}


float TUTracker::CalcBocmWeightedV4(const float k) const {
    FEATURE_LEVEL(1);
    return UFeature(CalcMatch(WordBocmWeightedV4Acc, k));
}


float TUTracker::CalcBocmWeightedV4W1(const float k) const {
    FEATURE_LEVEL(1);
    return UFeature(CalcMatchW1(WordBocmWeightedV4Acc, k));
}


float TUTracker::CalcBocmWeightedV4W2(const float k) const {
    FEATURE_LEVEL(2);
    return UFeature(CalcMatchW2(WordBocmWeightedV4Acc, k));
}


float TUTracker::CalcBocmWeightedMax(const float k) const {
    FEATURE_LEVEL(0);
    return UFeature(CalcMatchMax(WordBocmWeightedAcc, k));
}


float TUTracker::CalcBocmWeightedV2Max(const float k) const {
    FEATURE_LEVEL(1);
    return UFeature(CalcMatchMax(WordBocmWeightedV2Acc, k));
}


float TUTracker::CalcBocmDouble(const float k) const {
    FEATURE_LEVEL(1);
    return UFeature(CalcMatch(WordBocmDoubleAcc, k));
}


float TUTracker::CalcBocmDoubleW1(const float k) const {
    FEATURE_LEVEL(1);
    return UFeature(CalcMatchW1(WordBocmDoubleAcc, k));
}


float TUTracker::CalcBocmDoubleW2(const float k) const {
    FEATURE_LEVEL(2);
    return UFeature(CalcMatchW2(WordBocmDoubleAcc, k));
}


float TUTracker::CalcBocmDoubleV2(const float k) const {
    FEATURE_LEVEL(2);
    return UFeature(CalcMatch(WordBocmDoubleV2Acc, k));
}


float TUTracker::CalcBocmDoubleV2W1(const float k) const {
    FEATURE_LEVEL(2);
    return UFeature(CalcMatchW1(WordBocmDoubleV2Acc, k));
}


float TUTracker::CalcBocmDoubleV2W2(const float k) const {
    FEATURE_LEVEL(2);
    return UFeature(CalcMatchW2(WordBocmDoubleV2Acc, k));
}


float TUTracker::CalcBocmDoubleMax(const float k) const {
    FEATURE_LEVEL(1);
    return UFeature(CalcMatchMax(WordBocmDoubleAcc, k));
}


float TUTracker::CalcBocmDoubleV2Max(const float k) const {
    FEATURE_LEVEL(2);
    return UFeature(CalcMatchMax(WordBocmDoubleV2Acc, k));
}

#undef FEATURE_LEVEL

float TUTracker::CalcWordValueMax() const {
    float result = 0;
    for (auto i = 0u; i < QueryWordCount; ++i) {
        result += WordMaxValue[i] * WordRelWeight[i];
    }
    return UFeature(result);
}

TUTrackerWrapper::TUTrackerWrapper()
    : LastBreak(InvalidBreak)
{
}

void TUTrackerWrapper::Init(const TVector<float>& wordWeights)
{
    UTracker.Reset(new TUTracker(wordWeights));
}

bool TUTrackerWrapper::IsInited() const
{
    return (UTracker.Get() != nullptr);
}
