#include "query_model.h"

namespace NFacts {
    const TUtf16String SPACE = u" ";

    TUtf16String GetNormText(const TVector<TTypedLemmedToken>& queryTokens) {
        TUtf16String result;
        for (const TTypedLemmedToken& token : queryTokens) {
            if (result) {
                result.append(SPACE);
            }
            result.append(SubstNumericTokenOrGetLemma(token));
        }
        return result;
    }

    float CalculateQueryModelFeature(const TVector<TTypedLemmedToken>& queryTokens, const NEthos::TBinaryTextClassifierModel* model) {
        if (!model) {
            return 0.0;
        }
        const TUtf16String normalizedRequest = GetNormText(queryTokens);
        return model->Apply(normalizedRequest).Prediction;
    }
}
