#pragma once

#include <kernel/text_machine/parts/common/weights.h>
#include <kernel/text_machine/parts/common/storage.h>
#include <kernel/text_machine/interface/hit.h>

#include <ysite/yandex/relevance/cyclic_deque.h>

#include <kernel/text_machine/module/save_to_json.h>

namespace NTextMachine {
namespace NCore {
    class TMinWindow : public NModule::TJsonSerializable {
    public:
        TMinWindow() {
        }

        void Init(TMemoryPool& pool, ui32 queryLength);
        void NewDoc();
        void AddHit(ui16 wordId, ui32 offset);

        //(QueryLen - 1) / (MinWindowSize - 1)
        //This formula is more stable than QueryLen / MinWindowSize for different length of query,
        //because of min window first and last elements are always from query
        float CalcMinWindowSize() const;
        float CalcFullMatchCoverageMinOffset() const;
        float CalcFullMatchCoverageStartingPercent(ui32 docLength) const;

        void SaveToJson(NJson::TJsonValue& value) const;

    private:
        void Recalculate(ui32 offset);

        static const ui32 FALSE_OFFSET;

        ui32 QueryLength = Max<ui32>();
        TPoolPodHolder<ui32> MaxWordOffsets;
        ui32 MinWordOffset = Max<ui32>();

        ui32 MinWindowSize = Max<ui32>();
        ui32 FullMatchCoverageMinOffset = Max<ui32>();
    };

    class TWindowAccumulator
        : public NModule::TJsonSerializable
    {
    public:
        TWindowAccumulator(ui32 attenuationCoef)
            : AttenuationCoef(attenuationCoef)
        {
        }

        void Init(TMemoryPool& pool, size_t queryLength, TWeights& weights);
        void NewDoc();
        void AddHit(ui16 wordId);
        void RemoveHit(ui16 wordId);
        void Recalculate(ui32 movingWindowStart);
        void SaveToJson(NJson::TJsonValue& value) const;

        float CalcMaxWcm() const;
        float CalcMaxWcmAttenuation() const;
        float CalcWcmFullAttenuation() const;
        float CalcWcm95Attenuation() const;
        float CalcWcm80Attenuation() const;

    private:
        float MaxWcm = NAN;
        float MaxWcmAttenuation = NAN;
        float WcmFullAttenuation = NAN;
        float Wcm95Attenuation = NAN;
        float Wcm80Attenuation = NAN;

        TPoolPodHolder<size_t> WordCount;
        TWeights WordWeights;
        size_t QueryLength = Max<size_t>();
        float WindowWmc = NAN;
        ui32 AttenuationCoef = Max<ui32>();
    };

    class TMovingWindow
        : public NModule::TJsonSerializable
    {
    public:
        TMovingWindow(ui32 windowSize, ui32 attenuationCoef)
            : WindowAcc(attenuationCoef)
            , IsFixedWindowSize(windowSize > 0)
            , WindowSize(windowSize)
        {
        }

        void Init(TMemoryPool& pool, size_t queryLength, TWeights& weights);
        void NewDoc();
        void AddHit(ui16 wordId, ui32 offset);
        void SaveToJson(NJson::TJsonValue& value) const;

        TWindowAccumulator WindowAcc;

    private:
        struct TOffset {
            ui16 WordIdx = Max<ui16>();
            ui32 Pos = Max<ui32>();

            TOffset() {
            }

            TOffset(ui16 wordIdx, ui32 pos)
                : WordIdx(wordIdx)
                , Pos(pos)
            {
            }
        };

        TCyclicDeque<TOffset, 128> Offsets;
        bool IsFixedWindowSize = false;
        size_t WindowSize = Max<size_t>();
    };
}; // NCore
}; // NTextMachine
