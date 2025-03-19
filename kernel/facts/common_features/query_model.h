#pragma once

#include "query_tokens.h"

#include <kernel/ethos/lib/text_classifier/binary_classifier.h>


namespace NFacts {
    float CalculateQueryModelFeature(const TVector<TTypedLemmedToken>& queryTokens, const NEthos::TBinaryTextClassifierModel* model);
}
