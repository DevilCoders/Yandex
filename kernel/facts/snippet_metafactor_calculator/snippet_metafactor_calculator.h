#pragma once

#include "common.h"

#include <util/generic/vector.h>

namespace NFactsSnippetFactors {
    enum EMetafactorTechnique {
        MT_DIFF_TO_AVERAGE,
        MT_AVERAGE_DELTA
    };
}

namespace NFacts {
    TVector<float> CalculateSnippetMetaFactor(const TVector<float>& snippetFactors, NFactsSnippetFactors::EMetafactorTechnique metafactorTechnique);
    TVector<float> CalcDiffToConst(const TVector<float>& snippetFactors, float sub);
    TVector<float> AverageDelta(const TVector<float>& snippetFactors);
}

