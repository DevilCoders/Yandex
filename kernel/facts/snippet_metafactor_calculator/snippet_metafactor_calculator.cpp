#include "snippet_metafactor_calculator.h"

#include <util/generic/algorithm.h>
#include <util/generic/yexception.h>

namespace NFacts {
    constexpr float EPS = 0.0001;

    TVector<float> CalcDiffToConst(const TVector<float>& snippetFactors, float sub) {
        TVector<float> result(snippetFactors.size());
        Transform(snippetFactors.begin(), snippetFactors.end(), result.begin(), [sub](float x){return x - sub;});
        return result;
    }

    TVector<float> AverageDelta(const TVector<float>& snippetFactors) {
        if (snippetFactors.size() <= 1) {
            return TVector<float>(snippetFactors.size(), 0.f);
        }
        TVector<std::pair<float, size_t>> indexedSnippetFactors(snippetFactors.size());
        for (size_t i = 0; i < snippetFactors.size(); ++i) {
            indexedSnippetFactors[i] = std::make_pair(snippetFactors[i], i);
        }
        Sort(indexedSnippetFactors.begin(), indexedSnippetFactors.end());
        TVector<float> result(snippetFactors.size());
        for (size_t i = 0; i < snippetFactors.size(); ++i) {
            if (i > 0 && indexedSnippetFactors[i].first - indexedSnippetFactors[i - 1].first < EPS) {
                result[indexedSnippetFactors[i].second] = result[indexedSnippetFactors[i - 1].second];
                continue;
            }
            size_t iPlus = i;
            while (iPlus + 1 < snippetFactors.size()) {
                if (indexedSnippetFactors[iPlus].first - indexedSnippetFactors[i].first < EPS) {
                    ++iPlus;
                } else {
                    break;
                }
            }
            size_t iMinus = i;
            while (iMinus > 0) {
                if (indexedSnippetFactors[i].first - indexedSnippetFactors[iMinus].first < EPS) {
                    --iMinus;
                } else {
                    break;
                }
            }
            if (iPlus > iMinus) {
                result[indexedSnippetFactors[i].second] = (indexedSnippetFactors[iPlus].first - indexedSnippetFactors[iMinus].first)
                    / (static_cast<float>(iPlus) - static_cast<float>(iMinus));
            } else {
                result[indexedSnippetFactors[i].second] = 0.f;
            }
        }
        return result;
    }

    TVector<float> CalculateSnippetMetaFactor(const TVector<float>& snippetFactors, NFactsSnippetFactors::EMetafactorTechnique metafactorTechnique) {
        Y_ENSURE(snippetFactors);
        switch (metafactorTechnique) {
           case NFactsSnippetFactors::MT_DIFF_TO_AVERAGE:
                return CalcDiffToConst(snippetFactors, CalcMean(snippetFactors));
           case NFactsSnippetFactors::MT_AVERAGE_DELTA:
                return AverageDelta(snippetFactors);
           default:
               Y_ENSURE(false, "any snippet metafactor is not in release");
       }
    }

}
