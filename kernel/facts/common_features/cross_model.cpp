#include "cross_model.h"

namespace NFacts {
    static const TUtf16String WORD_SEPARATOR = u"__";

    TUtf16String GetBigram(const TTypedLemmedToken& queryToken, const TTypedLemmedToken& factToken) {
        return SubstNumericTokenOrGetLemma(queryToken) + WORD_SEPARATOR + SubstNumericTokenOrGetLemma(factToken);
    }

    bool IsPunct(const NFacts::TTypedLemmedToken& token) {
        return !TTokenizerSplitParams::NOT_PUNCT.SafeTest(token.Type);
    }

    float CalculateCrossModelFeature(const TVector<TTypedLemmedToken>& queryTokens, const TVector<TTypedLemmedToken>& answerTokens, const NEthos::TBinaryTextClassifierModel* crossModel)
    {
        if (!crossModel) {
            return 0;
        }

        float result = 0;
        for (const TTypedLemmedToken& factToken : answerTokens) {
            if (IsPunct(factToken))
                continue;
            for (const TTypedLemmedToken& queryToken : queryTokens) {
                result += crossModel->Apply(GetBigram(queryToken, factToken)).Prediction;
            }
        }
        return result;
    }
}
