#include <util/generic/singleton.h>
#include <util/generic/ptr.h>
#include <util/generic/hash.h>
#include <util/generic/noncopyable.h>

#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/system/mutex.h>
#include <util/system/guard.h>

#include "classifiers/defined_classifiers.h"

#include "token_classifiers_singleton.h"

namespace NTokenClassification {
    class TTokenClassifiersSingleton::TImpl: public TNonCopyable {
    public:
        TImpl()
            : IsInitialized(0)
        {
        }

        inline TTokenTypes Classify(const wchar16* tokenBegin,
                                    size_t length) const {
            return TokenClassifiers.Classify(tokenBegin, length);
        }

        inline void GetMarkup(const wchar16* tokenBegin,
                              size_t length,
                              ETokenType type,
                              TMultitokenMarkupGroups& markupGroups) const {
            TokenClassifiers.GetMarkup(tokenBegin, length, type, markupGroups);
        }

        inline void RegisterTokenClassifier(ITokenClassifier* tokenClassifier,
                                            ETokenType type) {
            TokenClassifiers.RegisterTokenClassifier(tokenClassifier, type);
        }

        TAtomic IsInitialized;
        TMutex Mutex;

        TTokenClassifiers TokenClassifiers;
    };

    TTokenTypes
    TTokenClassifiersSingleton::Classify(const wchar16* tokenBegin, size_t length) {
        TImpl* impl = Singleton<TImpl>();

        if (!AtomicGet(impl->IsInitialized)) {
            Initialize();
        }

        return impl->Classify(tokenBegin, length);
    }

    void TTokenClassifiersSingleton::GetMarkup(const wchar16* tokenBegin,
                                               size_t length,
                                               ETokenType type,
                                               TMultitokenMarkupGroups& markupGroups) {
        TImpl* impl = Singleton<TImpl>();

        if (!AtomicGet(impl->IsInitialized)) {
            Initialize();
        }

        impl->GetMarkup(tokenBegin, length, type, markupGroups);
    }

    void TTokenClassifiersSingleton::Initialize() {
        TImpl* impl = Singleton<TImpl>();

        TGuard<TMutex> guard(impl->Mutex);
        if (AtomicGet(impl->IsInitialized)) {
            return;
        }

        impl->RegisterTokenClassifier(new NEmailClassification::TEmailClassifier(), ETT_EMAIL);
        impl->RegisterTokenClassifier(new NUrlClassification::TUrlClassifier(), ETT_URL);
        impl->RegisterTokenClassifier(new NPunycodeClassification::TPunycodeClassifier(), ETT_PUNYCODE);

        AtomicSet(impl->IsInitialized, 1);
    }

}
