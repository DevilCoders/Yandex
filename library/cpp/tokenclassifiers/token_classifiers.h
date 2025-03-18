#pragma once

#include <util/generic/noncopyable.h>
#include <util/generic/hash.h>

#include "token_classifier_interface.h"
#include "token_types.h"
#include "token_markup.h"

namespace NTokenClassification {
    class TTokenClassifiers: public TNonCopyable {
    public:
        typedef TAutoPtr<ITokenClassifier> TTokenClassifierPtr;
        typedef THashMap<ETokenType, TTokenClassifierPtr> TTokenClassifierMap;

        typedef TTokenClassifierMap::iterator TTokenClassifierMapIterator;
        typedef TTokenClassifierMap::const_iterator TTokenClassifierMapConstIterator;

        TTokenClassifiers();

        TTokenTypes Classify(const wchar16* tokenBegin,
                             size_t length) const;

        void GetMarkup(const wchar16* tokenBegin,
                       size_t length,
                       ETokenType type,
                       TMultitokenMarkupGroups& markupGroups) const;

        void RegisterTokenClassifier(ITokenClassifier* tokenClassifier,
                                     ETokenType type);

    private:
        TTokenClassifierMap TokenClassifierMap;
    };

}
