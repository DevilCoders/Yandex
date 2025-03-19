#include "window_accumulator.h"

#include <kernel/text_machine/parts/common/types.h>

#include <kernel/text_machine/module/module_def.inc>

namespace NTextMachine {
namespace NCore {
    const ui32 TMinWindow::FALSE_OFFSET = Max<ui32>();

    void TMinWindow::Init(TMemoryPool& pool, ui32 queryLength)
    {
        QueryLength = queryLength;
        MaxWordOffsets.Init(pool, QueryLength, EStorageMode::Full);
    }

    void TMinWindow::NewDoc()
    {
        for (ui32 i = 0; i != QueryLength; ++i) {
            MaxWordOffsets[i] = FALSE_OFFSET;
        }
        MinWordOffset = FALSE_OFFSET;
        MinWindowSize = FALSE_OFFSET;
        FullMatchCoverageMinOffset = FALSE_OFFSET;
    }

    void TMinWindow::Recalculate(ui32 offset) {
        ui32 newMinWordOffset = FALSE_OFFSET;
        bool allCovered = true;
        for (ui32 i = 0; i != QueryLength; ++i) {
            if (MaxWordOffsets[i] != FALSE_OFFSET) {
                newMinWordOffset = Min(newMinWordOffset, MaxWordOffsets[i]);
            } else {
                allCovered = false;
            }
        }

        if (allCovered && (MinWordOffset == FALSE_OFFSET || newMinWordOffset > MinWordOffset)) {
            if (MinWordOffset == FALSE_OFFSET) {
                FullMatchCoverageMinOffset = offset;
            }
            MinWordOffset = newMinWordOffset;

            Y_ASSERT(MinWordOffset <= offset);
            MinWindowSize = Min(MinWindowSize, offset - MinWordOffset + 1);
        }
    }

    void TMinWindow::AddHit(ui16 wordId, ui32 offset) {
        if (MaxWordOffsets[wordId] == FALSE_OFFSET) {
            Y_ASSERT(MinWordOffset == FALSE_OFFSET);
            MaxWordOffsets[wordId] = offset;
            Recalculate(offset);
        } else if (MaxWordOffsets[wordId] == MinWordOffset) {
            Y_ASSERT(MaxWordOffsets[wordId] <= offset);
            MaxWordOffsets[wordId] = offset;
            Recalculate(offset);
        } else {
            Y_ASSERT(MaxWordOffsets[wordId] <= offset);
            MaxWordOffsets[wordId] = offset;
        }
    }

    float TMinWindow::CalcMinWindowSize() const {
        if (MinWindowSize == FALSE_OFFSET) {
            return 0.0f;
        }
        Y_ASSERT(MinWindowSize >= 1);
        if (QueryLength == 1) {
            return 1.0f;
        }

        return ((float)QueryLength - 1.0f) / ((float)Max(MinWindowSize, QueryLength) - 1.0f);
    }

    float TMinWindow::CalcFullMatchCoverageMinOffset() const {
        if (FullMatchCoverageMinOffset == FALSE_OFFSET) {
            return 0.0f;
        }

        return (float)QueryLength / (float)Max(FullMatchCoverageMinOffset, QueryLength);
    }

    float TMinWindow::CalcFullMatchCoverageStartingPercent(ui32 docLength) const {
        if (FullMatchCoverageMinOffset == FALSE_OFFSET) {
            return 0.0f;
        }

        return 1.0f - (float)FullMatchCoverageMinOffset / (float)Max(FullMatchCoverageMinOffset, docLength);
    }

    void TMinWindow::SaveToJson(NJson::TJsonValue& value) const {
        SAVE_JSON_VAR(value, QueryLength);
        SAVE_JSON_VAR(value, MaxWordOffsets);
        SAVE_JSON_VAR(value, MinWordOffset);
        SAVE_JSON_VAR(value, MinWindowSize);
        SAVE_JSON_VAR(value, FullMatchCoverageMinOffset);
    }

    void TWindowAccumulator::Init(TMemoryPool& pool, size_t queryLength, TWeights& weights) {
        Y_ASSERT(weights.Count() == queryLength);
        WordWeights = weights;
        QueryLength = queryLength;
        WordCount.Init(pool, QueryLength, EStorageMode::Full);
    }

    void TWindowAccumulator::NewDoc() {
        for (size_t i = 0; i < QueryLength; ++i) {
            WordCount[i] = 0;
        }
        WindowWmc = 0.0f;

        MaxWcm = 0.0f;
        MaxWcmAttenuation = 0.0f;
        WcmFullAttenuation = 0.0f;
        Wcm95Attenuation = 0.0f;
        Wcm80Attenuation = 0.0f;
    }

    void TWindowAccumulator::AddHit(ui16 wordId) {
        Y_ASSERT(wordId < QueryLength);
        if (!WordCount[wordId]) {
            WindowWmc += WordWeights[wordId];
            Y_ASSERT(WindowWmc < 1.0f + FloatEpsilon);
        }
        ++WordCount[wordId];
    }

    void TWindowAccumulator::RemoveHit(ui16 wordId) {
        Y_ASSERT(WordCount[wordId] > 0);
        Y_ASSERT(wordId < QueryLength);
        --WordCount[wordId];
        if (!WordCount[wordId]) {
            WindowWmc -= WordWeights[wordId];
            Y_ASSERT(WindowWmc > -FloatEpsilon);
        }
    }

    void TWindowAccumulator::Recalculate(ui32 movingWindowStart) {
        float attenuation = (float)AttenuationCoef / ((float)AttenuationCoef + (float)movingWindowStart);

        MaxWcm = Max(MaxWcm, WindowWmc);
        MaxWcmAttenuation = Max(MaxWcmAttenuation, attenuation * WindowWmc);
        if (WindowWmc > 0.8f) {
            Wcm80Attenuation = Max(Wcm80Attenuation, attenuation);
            if (WindowWmc > 0.95f) {
                Wcm95Attenuation = Max(Wcm95Attenuation, attenuation);
                if (WindowWmc > 0.99999f) {
                    WcmFullAttenuation = Max(WcmFullAttenuation, attenuation);
                }
            }
        }
    }

    float TWindowAccumulator::CalcMaxWcm() const {
        return MaxWcm;
    }

    float TWindowAccumulator::CalcMaxWcmAttenuation() const {
        return MaxWcmAttenuation;
    }

    float TWindowAccumulator::CalcWcmFullAttenuation() const {
        return WcmFullAttenuation;
    }

    float TWindowAccumulator::CalcWcm95Attenuation() const {
        return Wcm95Attenuation;
    }

    float TWindowAccumulator::CalcWcm80Attenuation() const {
        return Wcm80Attenuation;
    }

    void TWindowAccumulator::SaveToJson(NJson::TJsonValue& value) const {
        SAVE_JSON_VAR(value, MaxWcm);
        SAVE_JSON_VAR(value, MaxWcmAttenuation);
        SAVE_JSON_VAR(value, WcmFullAttenuation);
        SAVE_JSON_VAR(value, Wcm95Attenuation);
        SAVE_JSON_VAR(value, Wcm80Attenuation);
        SAVE_JSON_VAR(value, WordCount);
        SAVE_JSON_VAR(value, WordWeights);
        SAVE_JSON_VAR(value, QueryLength);
        SAVE_JSON_VAR(value, WindowWmc);
        SAVE_JSON_VAR(value, AttenuationCoef);
    }


    void TMovingWindow::Init(TMemoryPool& pool, size_t queryLength, TWeights& weights) {
        WindowAcc.Init(pool, queryLength, weights);
        if (!IsFixedWindowSize) {
            WindowSize = queryLength + 5;
        }
    }

    void TMovingWindow::NewDoc() {
        Offsets.Clear();
        WindowAcc.NewDoc();
    }

    void TMovingWindow::AddHit(ui16 wordId, ui32 offset) {
        while (!Offsets.Empty() && Offsets.Front().Pos + WindowSize <= offset) {
            const ui16 lastIdx = Offsets.Front().WordIdx;
            WindowAcc.RemoveHit(lastIdx);
            Offsets.PopFront();
        }

        if (Offsets.Full()) {
            return;
        }
        Offsets.PushBack(TOffset(wordId, offset));
        WindowAcc.AddHit(wordId);
        ui32 movingWindowStart = (offset > WindowSize) ? offset - WindowSize : 0;
        WindowAcc.Recalculate(movingWindowStart);
    }

    void TMovingWindow::SaveToJson(NJson::TJsonValue& value) const {
        SAVE_JSON_VAR(value, WindowAcc);
        SAVE_JSON_VAR(value, IsFixedWindowSize);
        SAVE_JSON_VAR(value, WindowSize);
    }
}; // NCore
}; // NTextMachine
