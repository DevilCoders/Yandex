#pragma once

#include <util/generic/noncopyable.h>

#include "token_classifier_interface.h"
#include "token_classifiers.h"
#include "token_types.h"
#include "token_markup.h"

namespace NTokenClassification {
    class TTokenClassifiersSingleton {
    public:
        static TTokenTypes Classify(const wchar16* tokenBegin, size_t length);

        static void GetMarkup(const wchar16* tokenBegin,
                              size_t length,
                              ETokenType type,
                              TMultitokenMarkupGroups& markupGroups);

    private:
        class TImpl;

        static void Initialize();
    };

}
