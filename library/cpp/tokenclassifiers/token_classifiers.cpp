#include <util/generic/singleton.h>
#include <util/generic/ptr.h>
#include <util/generic/hash.h>
#include <util/generic/noncopyable.h>

#include <util/system/mutex.h>
#include <util/system/guard.h>

#include "token_classifiers.h"

namespace NTokenClassification {
    TTokenClassifiers::TTokenClassifiers() {
    }

    TTokenTypes TTokenClassifiers::Classify(const wchar16* tokenBegin,
                                            size_t length) const {
        TTokenTypes types = 0;
        for (const auto& classifier : TokenClassifierMap) {
            types |= static_cast<TTokenTypes>(classifier.second->Classify(tokenBegin, length));
        }

        return types;
    }

    void TTokenClassifiers::GetMarkup(const wchar16* tokenBegin,
                                      size_t length,
                                      ETokenType type,
                                      TMultitokenMarkupGroups& markupGroups) const {
        TTokenClassifierMapConstIterator classifier = TokenClassifierMap.find(type);
        Y_ASSERT(classifier != TokenClassifierMap.end() &&
                 classifier->second.Get() != nullptr);
        classifier->second->GetMarkup(tokenBegin, length, markupGroups);
    }

    void TTokenClassifiers::RegisterTokenClassifier(ITokenClassifier* tokenClassifier,
                                                    ETokenType type) {
        typedef std::pair<ETokenType, TTokenClassifierPtr> TTokenClassifierMapEntry;

        Y_ASSERT(TokenClassifierMap.find(type) == TokenClassifierMap.end());
        TokenClassifierMap.insert(TTokenClassifierMapEntry(type,
                                                           TTokenClassifierPtr(tokenClassifier)));
    }

}
