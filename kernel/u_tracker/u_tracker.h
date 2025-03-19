#pragma once

#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"

#include <kernel/keyinv/hitlist/full_pos.h>

#include <util/generic/algorithm.h>
#include <util/generic/vector.h>
#include <util/generic/ymath.h>
#include <util/system/defaults.h>

class TUTracker;

class TUTrackerWrapper {
public:
    static const ui32 InvalidBreak = -1;
    static constexpr ui8 MaxPossibleCalcLevel = 3;
protected:
    THolder<class TUTracker> UTracker;
    ui32 LastBreak;
public:
    TUTrackerWrapper();
    virtual ~TUTrackerWrapper() = default;

    virtual float GetWeight(ui32 data) const = 0;

    virtual size_t GetFieldId() const = 0;

    virtual TString GetFieldName() const = 0;

    void Init(const TVector<float>& wordWeights);
    bool IsInited() const;
    virtual void InitNextDoc() = 0;

    // Old fashion style.
    virtual void AddHit(ui32 leftBreak, ui32 breakLength, const TFullPosition& pos, size_t wordIdx, ui32 data) = 0;

    // New fashion style. All positions with same leftBreak.
    virtual void AddSentence(ui32 leftBreak,
                     ui32 breakLength,
                     const TFullPositionEx* posBegin,
                     const TFullPositionEx* posEnd,
                     ui32 data
                     ) = 0;

    // Finishes features calculation and return raw TUTracker.
    virtual const TUTracker* FinishDoc() = 0;
};

template<ui8 MaxCalcLevel = TUTrackerWrapper::MaxPossibleCalcLevel>
class TUTrackerWrapperWithLevel : public TUTrackerWrapper {
public:
    // Old fashion style.
    void AddHit(ui32 leftBreak, ui32 breakLength, const TFullPosition& pos, size_t wordIdx, ui32 data) final;

    // New fashion style. All positions with same leftBreak.
    void AddSentence(ui32 leftBreak,
                     ui32 breakLength,
                     const TFullPositionEx* posBegin,
                     const TFullPositionEx* posEnd,
                     ui32 data) final;

    const TUTracker* FinishDoc() final;

    void InitNextDoc() final;
};

class TUTracker {
public:
    enum EFeature {
        FBm15,
        FBm15W1,
        FBm15W2,
        FBm15V0,
        FBm15V0W1,
        FBm15V0W2,
        FBm15V2,
        FBm15V2W1,
        FBm15V2W2,
        FBm15V4,
        FBm15V4W1,
        FBm15V4W2,
        FBm15VA,
        FBm15VAW1,
        FBm15VAW2,
        FBm15Strict,
        FBm15StrictW1,
        FBm15StrictW2,
        FBm15Max,
        FBm15V2Max,
        FBm15Atten,
        FBm15AttenW1,
        FBm15AttenW2,
        FBm15Wcm,
        FBm15WcmW1,
        FBm15WcmW2,
        FBm15Coverage,
        FBm15CoverageW1,
        FBm15CoverageV2,
        FBm15CoverageV2W1,
        FBm15CoverageV4,
        FBm15CoverageV4W1,
        FBclmPlain,
        FBclmPlainW1,
        FBclmPlainW2,
        FBclmPlainV2,
        FBclmPlainV2W1,
        FBclmPlainV2W2,
        FBclmMixPlain,
        FBclmMixPlainW1,
        FBclmMixPlainV2,
        FBclmMixPlainV2W1,
        FBclmWeighted,
        FBclmWeightedW1,
        FBclmWeightedW2,
        FBclmWeightedV2,
        FBclmWeightedV2W1,
        FBclmWeightedV2W2,
        FBclmMixWeighted,
        FBclmSoft,
        FBclmHard,
        FBclmHardW1,
        FBclmHardW2,
        FBocmPlain,
        FBocmWeighted,
        FBocmWeightedW1,
        FBocmWeightedW2,
        FBocmWeightedV2,
        FBocmWeightedV2W1,
        FBocmWeightedV2W2,
        FBocmWeightedV4,
        FBocmWeightedV4W1,
        FBocmWeightedV4W2,
        FBocmWeightedMax,
        FBocmWeightedV2Max,
        FBocmDouble,
        FBocmDoubleW1,
        FBocmDoubleW2,
        FBocmDoubleV2,
        FBocmDoubleV2W1,
        FBocmDoubleV2W2,
        FBocmDoubleMax,
        FBocmDoubleV2Max,

        FFullMatchPrediction,
        FSynonymMatchPrediction,
        FAnnotationMatchPrediction,
        FQueryMatchPrediction,
        FAnnotationMatchPredictionWeighted,

        FNumOfFeatures
    };

private:
    const ui32 QueryWordCount;
    static constexpr auto MaxWords = WORD_LEVEL_Max + 1;

    float WordAbsWeight[MaxWords]; // level >=0
    float WordRelWeight[MaxWords]; // level >=0
    float WordRel2Weight[MaxWords]; // level >=2
    ui32 SentWords[MaxWords]; // level >=0
    float WordDeviation[MaxWords]; // level >=0

    float WordVAAcc[MaxWords]; // level >=3
    float WordV0Acc[MaxWords]; // level >=2
    float WordAttenAcc[MaxWords]; // level >=2
    float WordBclmSoftAcc[MaxWords]; // level >=2
    float WordBclmHardAcc[MaxWords]; // level >=2
    float WordBocmDoubleV2Acc[MaxWords]; // level >=2
    float WordMaxValue[MaxWords]; // level >=2
    float PcmMax; // level >=2
    float ValuePcmMax; // level >=2
    float PcmSum; // level >=2
    float ValuePcmSum; // level >=2
    float WcmCoverageMax; // level >=2
    float ValueWcmCoverageMax; // level >=2
    float PrefixMatchMax; // level >=2
    float PrefixMatchSum; // level >=2
    ui32 PrefixMatchCount; // level >=2
    float SuffixMatchMax; // level >=2
    float SuffixMatchSum; // level >=2
    ui32 SuffixMatchCount; // level >=2
    float WordV2Acc[MaxWords]; // level >=1
    float WordV4Acc[MaxWords]; // level >=1
    float WordWcmAcc[MaxWords]; // level >=1
    float WordCoverageAcc[MaxWords]; // level >=1
    float WordCoverageV2Acc[MaxWords]; // level >=1
    float WordCoverageV4Acc[MaxWords]; // level >=1
    float WordBclmPlainV2Acc[MaxWords]; // level >=1
    float WordBclmWeightedV2Acc[MaxWords]; // level >=1
    float WordBocmPlainAcc[MaxWords]; // level >=1
    float WordBocmWeightedV2Acc[MaxWords]; // level >=1
    float WordBocmWeightedV4Acc[MaxWords]; // level >=1
    float WordBocmDoubleAcc[MaxWords]; // level >=1
    float WordAcc[MaxWords]; // level >=0
    float WordStrictAcc[MaxWords]; // level >=0
    float WordBclmPlainAcc[MaxWords]; // level >=0
    float WordBclmWeightedAcc[MaxWords]; // level >=0
    float WordBocmWeightedAcc[MaxWords]; // level >=0
    ui32 LastSent[MaxWords]; // level >=0
    float WcmMax; // level >=0
    float ValueWcmMax; // level >=0
    float WcmSum; // level >=0
    float ValueWcmSum; // level >=0
    float FullMatchValue; // level >=0
    float SynonymMatchValue; // level >=0
    float AnnotationMatchValue; // level >=0
    float AnnotationMatchWeightedValue; // level >=0
    float QueryMatchValue; // level >=0

    float SentWeightedDeviationSum; // level >=2
    ui32 SentSuffixWordCount; // level >=2
    float SentValue2; // level >=1
    float SentValue4; // level >=1
    float SentDeviationSum; // level >=1
    float SentFormSum; // level >=1
    ui32 SentCount; // level >=0
    float SentValue; // level >=0
    ui32 SentLength; // level >=0
    float SentWcm; // level >=0
    ui32 SentWordCount; // level >=0
    ui32 FullMatchCount; // level >=0
    ui32 SynonymMatchCount; // level >=0
    ui32 SentCoverage; // level >=0
    float SentWeightSum; // level >=0

    ui32 LastPosition; // level >=0
    ui32 LastIdx; // level >=0

    bool IsSentStarted = false; // level >=0

    template<ui8 MaxCalcLevel>
    friend class TUTrackerWrapperWithLevel;

    Y_IF_DEBUG(
        ui8 DeclaredCalcLevel;
        mutable ui8 MaxCalledCalcLevel;
    )

private:
    struct TPrecalculatedTable {
        float Distance[MaxWords];
        float Deviation[MaxWords];
        float Attenuation[MaxWords];
        float GlobalAttenuation[1024];
        TPrecalculatedTable();
    };
    static const TPrecalculatedTable PrecalculatedTable;

private:

    float Y_FORCE_INLINE CalcTf(const float value, const float k) const noexcept;
    float CalcProximity(const ui32 leftPosition, const ui32 rightPosition) const noexcept;
    float CalcDeviation(const ui32 queryPosition, const ui32 hitPosition) const noexcept;
    float CalcAttenuation(const ui32 position) const noexcept;
    float CalcGlobalAttenuation(const ui32 position) const noexcept;
    float CalcFormWeightSoft(const TFullPosition& pos) const noexcept;
    float CalcFormWeightStrict(const TFullPosition& pos) const noexcept;
    float CalcMatch(const float* acc, const float k) const noexcept;
    float CalcMatchMax(const float* acc, const float k) const noexcept;
    float CalcMatchW1(const float* acc, const float k) const noexcept;
    float CalcMatchW2(const float* acc, const float k) const noexcept;
    float CalcMatchMix(const float* acc1, const float* acc2, const float alpha, const float k) const noexcept;
    float CalcMatchMixW1(const float* acc1, const float* acc2, const float alpha, const float k) const noexcept;
    float CalcBclm(const float* acc, const float k) const noexcept;
    float CalcBclmW1(const float* acc, const float k) const noexcept;
    float CalcBclmW2(const float* acc, const float k) const noexcept;
    ui32 GetLeftPosition(const TFullPosition& pos) const noexcept;
    ui32 GetRightPosition(const TFullPosition& pos) const noexcept;

    float CalcFeatureImpl(EFeature feature, const float k, const float alpha) const;

public:

    TUTracker(const TVector<float>& wordWeight);
    template<ui8 MaxCalcLevel = TUTrackerWrapper::MaxPossibleCalcLevel> void NewDoc();
    template<ui8 MaxCalcLevel = TUTrackerWrapper::MaxPossibleCalcLevel> void StartSent(const float value, const ui32 length = 0);
    template<ui8 MaxCalcLevel = TUTrackerWrapper::MaxPossibleCalcLevel> void AddHit(const TFullPosition& pos, const size_t wordIdx);
    template<ui8 MaxCalcLevel = TUTrackerWrapper::MaxPossibleCalcLevel> void FinishSent();

    float CalcFeature(EFeature feature, const float k, const float alpha) const;

    float CalcWcmMax() const;
    float CalcValueWcmMax() const;
    float CalcValueWcmAvg() const;
    float CalcValueWcmPrediction() const;
    float CalcPcmMax() const;
    float CalcValuePcmMax() const;
    float CalcValuePcmAvg() const;
    float CalcValuePcmPrediction() const;
    float CalcWordValueMax() const;
    float CalcFullMatchPrediction() const;
    float CalcSynonymMatchPrediction() const;
    float CalcAnnotationMatchPrediction() const;
    float CalcAnnotationMatchPredictionWeighted() const;
    float CalcQueryMatchPrediction() const;
    float CalcPrefixMatchMax() const;
    float CalcPrefixMatchAvg() const;
    float CalcPrefixMatchCount() const;
    float CalcSuffixMatchMax() const;
    float CalcSuffixMatchAvg() const;
    float CalcSuffixMatchCount() const;
    float CalcWcmCoveragePrediction() const;
    float CalcWcmCoverageMax() const;
    float CalcBm15(const float k) const;
    float CalcBm15W1(const float k) const;
    float CalcBm15W2(const float k) const;
    float CalcBm15V0(const float k) const;
    float CalcBm15V0W1(const float k) const;
    float CalcBm15V0W2(const float k) const;
    float CalcBm15V2(const float k) const;
    float CalcBm15V2W1(const float k) const;
    float CalcBm15V2W2(const float k) const;
    float CalcBm15V4(const float k) const;
    float CalcBm15V4W1(const float k) const;
    float CalcBm15V4W2(const float k) const;
    float CalcBm15VA(const float k) const;
    float CalcBm15VAW1(const float k) const;
    float CalcBm15VAW2(const float k) const;
    float CalcBm15Strict(const float k) const;
    float CalcBm15StrictW1(const float k) const;
    float CalcBm15StrictW2(const float k) const;
    float CalcBm15Max(const float k) const;
    float CalcBm15V2Max(const float k) const;
    float CalcBm15Atten(const float k) const;
    float CalcBm15AttenW1(const float k) const;
    float CalcBm15AttenW2(const float k) const;
    float CalcBm15Wcm(const float k) const;
    float CalcBm15WcmW1(const float k) const;
    float CalcBm15WcmW2(const float k) const;
    float CalcBm15Coverage(const float k) const;
    float CalcBm15CoverageW1(const float k) const;
    float CalcBm15CoverageV2(const float k) const;
    float CalcBm15CoverageV2W1(const float k) const;
    float CalcBm15CoverageV4(const float k) const;
    float CalcBm15CoverageV4W1(const float k) const;
    float CalcBclmPlain(const float k) const;
    float CalcBclmPlainW1(const float k) const;
    float CalcBclmPlainW2(const float k) const;
    float CalcBclmPlainV2(const float k) const;
    float CalcBclmPlainV2W1(const float k) const;
    float CalcBclmPlainV2W2(const float k) const;
    float CalcBclmMixPlain(const float k, const float alpha) const;
    float CalcBclmMixPlainW1(const float k, const float alpha) const;
    float CalcBclmMixPlainV2(const float k, const float alpha) const;
    float CalcBclmMixPlainV2W1(const float k, const float alpha) const;
    float CalcBclmWeighted(const float k) const;
    float CalcBclmWeightedW1(const float k) const;
    float CalcBclmWeightedW2(const float k) const;
    float CalcBclmWeightedV2(const float k) const;
    float CalcBclmWeightedV2W1(const float k) const;
    float CalcBclmWeightedV2W2(const float k) const;
    float CalcBclmMixWeighted(const float k, const float alpha) const;
    float CalcBclmSoft(const float k) const;
    float CalcBclmHard(const float k) const;
    float CalcBclmHardW1(const float k) const;
    float CalcBclmHardW2(const float k) const;
    float CalcBocmPlain(const float k) const;
    float CalcBocmWeighted(const float k) const;
    float CalcBocmWeightedW1(const float k) const;
    float CalcBocmWeightedW2(const float k) const;
    float CalcBocmWeightedV2(const float k) const;
    float CalcBocmWeightedV2W1(const float k) const;
    float CalcBocmWeightedV2W2(const float k) const;
    float CalcBocmWeightedV4(const float k) const;
    float CalcBocmWeightedV4W1(const float k) const;
    float CalcBocmWeightedV4W2(const float k) const;
    float CalcBocmWeightedMax(const float k) const;
    float CalcBocmWeightedV2Max(const float k) const;
    float CalcBocmDouble(const float k) const;
    float CalcBocmDoubleW1(const float k) const;
    float CalcBocmDoubleW2(const float k) const;
    float CalcBocmDoubleV2(const float k) const;
    float CalcBocmDoubleV2W1(const float k) const;
    float CalcBocmDoubleV2W2(const float k) const;
    float CalcBocmDoubleMax(const float k) const;
    float CalcBocmDoubleV2Max(const float k) const;
};

#define CASE_LEVEL(LEVEL) case LEVEL: Y_ASSERT(LEVEL <= MaxCalcLevel && "Failure with feature level");

template<ui8 MaxCalcLevel>
void TUTracker::NewDoc() {
    Y_IF_DEBUG(
                MaxCalledCalcLevel = 0;
                DeclaredCalcLevel = MaxCalcLevel;
                )

#define FILL_ZERO(ARR) {Fill(ARR, ARR + QueryWordCount, 0);}

    switch (MaxCalcLevel) {
    default:
    CASE_LEVEL(3)
        FILL_ZERO(WordVAAcc);

    CASE_LEVEL(2)
        FILL_ZERO(WordV0Acc);
        FILL_ZERO(WordAttenAcc);
        FILL_ZERO(WordBclmSoftAcc);
        FILL_ZERO(WordBclmHardAcc);
        FILL_ZERO(WordBocmDoubleV2Acc);
        FILL_ZERO(WordMaxValue);

        PcmMax = 0;
        ValuePcmMax = 0;
        PcmSum = 0;
        ValuePcmSum = 0;
        WcmCoverageMax = 0;
        ValueWcmCoverageMax = 0;
        PrefixMatchMax = 0;
        PrefixMatchSum = 0;
        PrefixMatchCount = 0;
        SuffixMatchMax = 0;
        SuffixMatchSum = 0;
        SuffixMatchCount = 0;

    CASE_LEVEL(1)
        FILL_ZERO(WordV2Acc);
        FILL_ZERO(WordV4Acc);
        FILL_ZERO(WordWcmAcc);
        FILL_ZERO(WordCoverageAcc);
        FILL_ZERO(WordCoverageV2Acc);
        FILL_ZERO(WordCoverageV4Acc);
        FILL_ZERO(WordBclmPlainV2Acc);
        FILL_ZERO(WordBclmWeightedV2Acc);
        FILL_ZERO(WordBocmPlainAcc);
        FILL_ZERO(WordBocmWeightedV2Acc);
        FILL_ZERO(WordBocmWeightedV4Acc);
        FILL_ZERO(WordBocmDoubleAcc);

    CASE_LEVEL(0)
        FILL_ZERO(WordAcc);
        FILL_ZERO(WordStrictAcc);
        FILL_ZERO(WordBclmPlainAcc);
        FILL_ZERO(WordBclmWeightedAcc);
        FILL_ZERO(WordBocmWeightedAcc);
        FILL_ZERO(LastSent);

        WcmMax = 0;
        ValueWcmMax = 0;
        WcmSum = 0;
        ValueWcmSum = 0;
        FullMatchValue = 0;
        SynonymMatchValue = 0;
        AnnotationMatchValue = 0;
        AnnotationMatchWeightedValue = 0;
        QueryMatchValue = 0;
        SentCount = 0;
    }

#undef FILL_ZERO

}

template<ui8 MaxCalcLevel>
void TUTracker::AddHit(const TFullPosition& pos, const size_t wordIdx) {
    Y_IF_DEBUG(
                Y_VERIFY(MaxCalcLevel == DeclaredCalcLevel, "wrong class usage: using variable MaxCalcLevel is forbidden");
                Y_ASSERT(IsSentStarted);
    )

    if (Y_UNLIKELY(wordIdx >= MaxWords)) {
        return;
    }

    const float strictForm = CalcFormWeightStrict(pos);
    const ui32 position = GetLeftPosition(pos);
    const ui32 maxPosition = (0 == SentWordCount) ? position : Max(position, LastPosition + 1);

    if (position == wordIdx) {
        ++SynonymMatchCount;
        if (strictForm > 0.5f) {
            ++FullMatchCount;
        }
    }

    if (
            MaxCalcLevel >= 2 &&
            SentLength >= QueryWordCount &&
            position == (SentLength - QueryWordCount + wordIdx) &&
            strictForm > 0.5f
            ) {
        ++SentSuffixWordCount;
    }

    const ui32 rightPosition = GetRightPosition(pos);

    if (maxPosition <= rightPosition) {
        SentCoverage += rightPosition + 1 - maxPosition;
    }

    if (Y_LIKELY(SentWordCount > 0 && wordIdx != LastIdx)) {
        const float proximity = CalcProximity(LastPosition, position);
        const float bclmWeight = SentValue * proximity;

        switch (MaxCalcLevel) {
        default:
        CASE_LEVEL(3)
        CASE_LEVEL(2)
            if (wordIdx == (LastIdx + 1)) {
                WordBclmSoftAcc[wordIdx] += bclmWeight;
                WordBclmSoftAcc[LastIdx] += bclmWeight;
                WordBclmHardAcc[wordIdx] += bclmWeight;
                WordBclmHardAcc[LastIdx] += bclmWeight;
            } else if ((wordIdx + 1) == LastIdx) {
                WordBclmSoftAcc[wordIdx] += bclmWeight;
                WordBclmSoftAcc[LastIdx] += bclmWeight;
            }

        CASE_LEVEL(1) {
            const float bclmWeight2 = SentValue2 * proximity;
            WordBclmPlainV2Acc[wordIdx] += bclmWeight2;
            WordBclmPlainV2Acc[LastIdx] += bclmWeight2;
            WordBclmWeightedV2Acc[wordIdx] += bclmWeight2 * WordAbsWeight[LastIdx];
            WordBclmWeightedV2Acc[LastIdx] += bclmWeight2 * WordAbsWeight[wordIdx];
        }

        CASE_LEVEL(0)
            WordBclmPlainAcc[wordIdx] += bclmWeight;
            WordBclmPlainAcc[LastIdx] += bclmWeight;
            WordBclmWeightedAcc[wordIdx] += bclmWeight * WordAbsWeight[LastIdx];
            WordBclmWeightedAcc[LastIdx] += bclmWeight * WordAbsWeight[wordIdx];
        }
    }

    const float form = CalcFormWeightSoft(pos);

    if (Y_LIKELY(LastSent[wordIdx] != SentCount)) {
        const float formValue = SentValue * form;
        const float formDeviation = form * CalcDeviation(wordIdx, position);

        switch (MaxCalcLevel) {
        default:
        CASE_LEVEL(3)
        CASE_LEVEL(2)
            WordV0Acc[wordIdx] += form;
            WordAttenAcc[wordIdx] += formValue * CalcAttenuation(position);
            if (WordMaxValue[wordIdx] < SentValue) {
                WordMaxValue[wordIdx] = SentValue;
            }
            SentWeightedDeviationSum += formDeviation * WordRelWeight[wordIdx];
        CASE_LEVEL(1)
            WordV2Acc[wordIdx] += SentValue2 * form;
            WordV4Acc[wordIdx] += SentValue4 * form;
            SentDeviationSum += formDeviation;
            SentFormSum += form;
        CASE_LEVEL(0)
            SentWords[SentWordCount] = wordIdx;
            WordDeviation[wordIdx] = formDeviation;
            WordAcc[wordIdx] += formValue;
            WordStrictAcc[wordIdx] += SentValue * strictForm;
            LastSent[wordIdx] = SentCount;
            SentWcm += WordRelWeight[wordIdx];
            ++SentWordCount;
            SentWeightSum += WordAbsWeight[wordIdx];
        }
    }

    switch (MaxCalcLevel) {
    default:
    CASE_LEVEL(3)
        WordVAAcc[wordIdx] += form * CalcGlobalAttenuation(TWordPosition::Break(pos.Beg));

    CASE_LEVEL(2)
    CASE_LEVEL(1)
    CASE_LEVEL(0)
        LastPosition = rightPosition;
        LastIdx = wordIdx;
    }
}

template<ui8 MaxCalcLevel>
void TUTracker::FinishSent() {
    Y_IF_DEBUG(
                Y_VERIFY(MaxCalcLevel == DeclaredCalcLevel, "wrong class usage: using variable MaxCalcLevel is forbidden");
                Y_VERIFY(IsSentStarted);
                )

    IsSentStarted = false;

    const float wcmValue = SentWcm * SentValue;
    const float coverage = float(SentCoverage) / float(Max(SentLength, SentCoverage));

    switch (MaxCalcLevel) {
    default:
    CASE_LEVEL(3)
    CASE_LEVEL(2) {
        if (SentLength > 0) {
            if (FullMatchCount >= QueryWordCount) {
                PrefixMatchMax = Max(PrefixMatchMax, SentValue);
                PrefixMatchSum += SentValue;
                ++PrefixMatchCount;
            }

            if (SentSuffixWordCount >= QueryWordCount) {
                SuffixMatchMax = Max(SuffixMatchMax, SentValue);
                SuffixMatchSum += SentValue;
                ++SuffixMatchCount;
            }
        }

        if (SentWeightedDeviationSum > PcmMax) {
            PcmMax = SentWeightedDeviationSum;
            ValuePcmMax = SentValue;
        } else if (SentWeightedDeviationSum == PcmMax && SentValue > ValuePcmMax) {
            ValuePcmMax = SentValue;
        }

        const float wcmCoverage = SentWcm * coverage;
        if (wcmCoverage > WcmCoverageMax) {
            WcmCoverageMax = wcmCoverage;
            ValueWcmCoverageMax = SentValue;
        } else if (wcmCoverage == WcmCoverageMax) {
            ValueWcmCoverageMax = Max(ValueWcmCoverageMax, SentValue);
        }

        PcmSum += SentWeightedDeviationSum;
        const float pcmValue = SentWeightedDeviationSum * SentValue;
        ValuePcmSum += SentWeightedDeviationSum * pcmValue;
    }
    CASE_LEVEL(1)
    CASE_LEVEL(0)
        if (FullMatchCount >= QueryWordCount) {
            QueryMatchValue = Max(QueryMatchValue, SentValue);
        }

        if (SentCoverage >= SentLength) {
            AnnotationMatchValue = Max(AnnotationMatchValue, SentValue);
            AnnotationMatchWeightedValue = Max(AnnotationMatchWeightedValue, wcmValue);
        }

        if (SentWcm > WcmMax) {
            WcmMax = SentWcm;
            ValueWcmMax = SentValue;
        } else if (SentWcm == WcmMax && SentValue > ValueWcmMax) {
            ValueWcmMax = SentValue;
        }

        if (SentCoverage >= SentLength && SentLength > 0) {
            if (FullMatchCount >= QueryWordCount) {
                FullMatchValue = Max(FullMatchValue, SentValue);
            }
            if (SynonymMatchCount >= QueryWordCount) {
                SynonymMatchValue = Max(SynonymMatchValue, SentValue);
            }
        }

        WcmSum += SentWcm;
        ValueWcmSum += SentWcm * wcmValue;
    }

    switch (MaxCalcLevel) {
    default:
    CASE_LEVEL(3)
    CASE_LEVEL(2)
        for (auto wordIdx = SentWords; wordIdx != SentWords + SentWordCount; ++wordIdx) {
            WordBocmDoubleV2Acc[*wordIdx] += SentValue2 * SentDeviationSum * WordDeviation[*wordIdx];
        }

    CASE_LEVEL(1) {
        for (auto wordIdx = SentWords; wordIdx != SentWords + SentWordCount; ++wordIdx) {
            WordWcmAcc[*wordIdx] += wcmValue;
        }

        const float coverageValue = coverage * SentValue;
        for (auto wordIdx = SentWords; wordIdx != SentWords + SentWordCount; ++wordIdx) {
            WordCoverageAcc[*wordIdx] += coverageValue;
        }

        const float coverageValue2 = coverage * SentValue2;
        for (auto wordIdx = SentWords; wordIdx != SentWords + SentWordCount; ++wordIdx) {
            WordCoverageV2Acc[*wordIdx] += coverageValue2;
        }

        const float coverageValue4 = coverage * SentValue4;
        for (auto wordIdx = SentWords; wordIdx != SentWords + SentWordCount; ++wordIdx) {
            WordCoverageV4Acc[*wordIdx] += coverageValue4;
        }

        const float formValue = SentFormSum * SentValue;
        for (auto wordIdx = SentWords; wordIdx != SentWords + SentWordCount; ++wordIdx) {
            WordBocmPlainAcc[*wordIdx] += formValue * WordDeviation[*wordIdx];
        }

        const float weightValue2 = SentWeightSum * SentValue2;
        for (auto wordIdx = SentWords; wordIdx != SentWords + SentWordCount; ++wordIdx) {
            WordBocmWeightedV2Acc[*wordIdx] += weightValue2 * WordDeviation[*wordIdx];
        }

        const float weightValue4 = SentWeightSum * SentValue4;
        for (auto wordIdx = SentWords; wordIdx != SentWords + SentWordCount; ++wordIdx) {
            WordBocmWeightedV4Acc[*wordIdx] += weightValue4 * WordDeviation[*wordIdx];
        }
        for (auto wordIdx = SentWords; wordIdx != SentWords + SentWordCount; ++wordIdx) {
            WordBocmDoubleAcc[*wordIdx] += SentValue * SentDeviationSum * WordDeviation[*wordIdx];
        }
    }

    CASE_LEVEL(0) {
        const float weightValue = SentWeightSum * SentValue;
        for (auto wordIdx = SentWords; wordIdx != SentWords + SentWordCount; ++wordIdx) {
            WordBocmWeightedAcc[*wordIdx] += weightValue * WordDeviation[*wordIdx];
        }
    }
    }
}

template<ui8 MaxCalcLevel>
void TUTracker::StartSent(const float value, const ui32 length) {
    Y_IF_DEBUG(
                Y_VERIFY(MaxCalcLevel == DeclaredCalcLevel, "wrong class usage: using variable MaxCalcLevel is forbidden");
            )

    switch (MaxCalcLevel) {
    default:
    CASE_LEVEL(3)
    CASE_LEVEL(2)
        SentWeightedDeviationSum = 0;
        SentSuffixWordCount = 0;

    CASE_LEVEL(1)
        SentValue2 = value * value * 10.f;
        SentValue4 = SentValue2 * SentValue2;
        SentDeviationSum = 0;
        SentFormSum = 0;

    CASE_LEVEL(0)
        ++SentCount;
        SentValue = value;
        SentLength = length;
        SentWcm = 0;
        SentWordCount = 0;
        FullMatchCount = 0;
        SynonymMatchCount = 0;
        SentCoverage = 0;
        IsSentStarted = true;
        SentWeightSum = 0;
    }
}

#undef CASE_LEVEL
