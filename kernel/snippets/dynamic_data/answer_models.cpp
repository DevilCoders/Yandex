#include "answer_models.h"

#include <kernel/ethos/lib/text_classifier/binary_classifier.h>

#include <util/generic/hash.h>
#include <util/memory/blob.h>

namespace NSnippets {

    class TAnswerModels::TImpl {
    private:
        NEthos::TBinaryTextClassifierModelsStorage Models;

    public:
        explicit TImpl(const TBlob& blob) {
            Models.LoadFromBlob(blob);
        }

        explicit TImpl(const TString& filename) {
            Models.LoadFromFile(filename);
        }

        float ApplyWord(const TUtf16String& word, ELanguage language) const {
            const auto* model = Models.ModelByLanguage(language);
            if (nullptr != model && !model->Empty()) {
                NEthos::TDocument document({ComputeHash(word)});
                return model->Apply(std::move(document)).Prediction;
            }
            return 0.0;
        }
    };

    TAnswerModels::TAnswerModels() {}

    TAnswerModels::TAnswerModels(const TBlob& blob)
        : Impl(new TImpl(blob))
    {}

    TAnswerModels::TAnswerModels(const TString& filename)
        : Impl(new TImpl(filename))
    {}

    void TAnswerModels::InitFromBlob(const TBlob& blob) {
        Impl = MakeHolder<TImpl>(blob);
    }

    void TAnswerModels::InitFromFilename(const TString& filename) {
        Impl = MakeHolder<TImpl>(filename);
    }

    bool TAnswerModels::Empty() const {
        return Impl.Get() == nullptr;
    }

    TAnswerModels::~TAnswerModels() {
    }

    float TAnswerModels::ApplyWord(const TUtf16String& word, ELanguage language) const {
        return Impl->ApplyWord(word, language);
    }

}
