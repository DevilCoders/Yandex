#pragma once
#include "query_tokens.h"

#include <kernel/ethos/lib/text_classifier/binary_classifier.h>

namespace NFacts {
    float CalculateCrossModelFeature(const TVector<TTypedLemmedToken>& queryTokens,
            const TVector<TTypedLemmedToken>& answerTokens,
            const NEthos::TBinaryTextClassifierModel* crossModel);
}


