#include "chain_accumulator.h"

#include <kernel/text_machine/module/module_def.inc>

namespace NTextMachine {
namespace NCore {
    void TChainAccumulator::NewQuery(TMemoryPool& pool,
        const TQuery* query,
        const TWeights& weights)
    {
        Y_ASSERT(query);
        Query = query;
        WordWeights = weights;
        Chain.Init(pool, Query->Words.size());
    }

    void TChainAccumulator::NewDoc()
    {
        MaxWcm = 0.0;
    }

    void TChainAccumulator::Start()
    {
        ChainWordWeight = 0.0;
        Chain.Clear();
    }

    float TChainAccumulator::CalcMaxWcm() const
    {
        return MaxWcm;
    }

    void TChainAccumulator::SaveToJson(NJson::TJsonValue& value) const
    {
        SAVE_JSON_VAR(value, Chain);
        SAVE_JSON_VAR(value, ChainStartPosition);
        SAVE_JSON_VAR(value, ChainWordWeight);
        SAVE_JSON_VAR(value, MaxWcm);
    }
} // NCore
} // NTextMachine
