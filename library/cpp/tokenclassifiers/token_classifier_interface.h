#pragma once

#include <util/generic/string.h>

#include "token_types.h"

namespace NTokenClassification {
    class TMultitokenMarkupGroups;

    class ITokenClassifier {
    public:
        virtual ~ITokenClassifier() {
        }

        virtual ETokenType Classify(const wchar16* tokenBegin, size_t length) const = 0;
        virtual void GetMarkup(const wchar16* tokenBegin,
                               size_t length,
                               TMultitokenMarkupGroups& markupGroups) const = 0;
    };

}
