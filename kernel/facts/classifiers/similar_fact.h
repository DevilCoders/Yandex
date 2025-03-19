#pragma once

#include <kernel/facts/factors_info/factor_names.h>
#include <util/generic/vector.h>

namespace NFactClassifiers
{
    struct TSimilarFact {
        TString Query;
        TString Answer;
        int SourceIndex = 0;
        float DotProduct = 0;
        TUtf16String NormedQuery;
        NUnstructuredFeatures::TFactFactorStorage Factors;
        float MatrixnetValue = 0;
        float MatrixnetThreshold = 0;

        TSimilarFact()
        {}

        TSimilarFact(const TString& query,
                     const TString& answer,
                     int sourceIndex,
                     float dotProduct,
                     const TUtf16String& normedQuery,
                     float matrixnetThreshold)
            : Query(query)
            , Answer(answer)
            , SourceIndex(sourceIndex)
            , DotProduct(dotProduct)
            , NormedQuery(normedQuery)
            , MatrixnetThreshold(matrixnetThreshold)
        {}
    };
}

