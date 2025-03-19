#pragma once

#include "calculator_data.h"
#include <kernel/facts/features_calculator/embeddings/embeddings.h>
#include <kernel/facts/features_calculator/idl/serp_data.pb.h>
#include <ml/neocortex/neocortex_lib/helpers.h>
#include <util/generic/maybe.h>

namespace NUnstructuredFeatures
{
    class TQueryCalculator;

    class TSerpCalculator {
    public:
        TSerpCalculator(const TCalculatorData& data, const TVector<TString>& serp, bool normalizeUrls = true);
        TSerpCalculator(const TCalculatorData& data, const TUnstructuredSerp& serp);

        void BuildIntersectionFeatures(const TSerpCalculator& rhs, TVector<float>& features) const;
        void BuildNeocortexFeatures(const TSerpCalculator& rhs, TVector<float>& features) const;
        void BuildNeocortexFeatures(const TQueryCalculator& rhs, TVector<float>& features) const;

        const TCalculatorData& GetData() const {
            return Data;
        }

    private:
        void EnsureTtHEmbed() const;

    private:
        const TCalculatorData &Data;

        struct TSerpStrings {
            TVector<TString> Vec;
            THashMap<TString, int> Positions;
            void Add(const TString &what);
        };
        TSerpStrings Urls, Hosts;
        mutable TEmbedding TextToHostsNeocortexEmbed;
    };
}
