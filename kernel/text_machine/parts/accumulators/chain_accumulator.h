#pragma once

#include "coordination_accumulator.h"

#include <kernel/text_machine/parts/common/weights.h>
#include <kernel/text_machine/parts/common/types.h>
#include <kernel/text_machine/interface/query.h>
#include <kernel/text_machine/module/save_to_json.h>

#include <util/system/defaults.h>

namespace NTextMachine {
namespace NCore {
    class TChainAccumulator
        : public NModule::TJsonSerializable
    {
    public:
        void NewQuery(TMemoryPool& pool, const TQuery* query, const TWeights& weights);
        void NewDoc();
        void Start();
        void Update(const ui16 wordId, const ui16 position);
        float CalcMaxWcm() const;

        // FIXME: Not yet implemented
        float CalcMaxWccm() const;
        float CalcMaxAwcm() const;
        float CalcMaxAwccm() const;
        float CalcFullWcmAttenuation() const;
        float CalcFullWcmCoverage() const;
        float Calc95WcmAttenuation() const;
        float Calc95WcmCoverage() const;
        float Calc80WcmAttenuation() const;
        float Calc80WcmCoverage() const;

        void SaveToJson(NJson::TJsonValue& value) const;

    private:
        const TQuery* Query = nullptr;
        TWeights WordWeights;
        TCoordinationAccumulator Chain;
        ui16 ChainStartPosition = Max<ui16>();
        float ChainWordWeight = NAN;
        float MaxWcm = NAN;
    };

    Y_FORCE_INLINE void TChainAccumulator::Update(const ui16 wordId, const ui16 position)
    {
        if (0 == Chain.Count()) {
            ChainStartPosition = position;
        }
        if (!Chain.Contain(wordId)) {
            ChainWordWeight += WordWeights[wordId];
            Y_ASSERT(ChainWordWeight < 1.0f + FloatEpsilon);
            MaxWcm = Max(MaxWcm, ChainWordWeight);
            Chain.Update(wordId);
        }
    }
} // NCore
} // NTextMachine
