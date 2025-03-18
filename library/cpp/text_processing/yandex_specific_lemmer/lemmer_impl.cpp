#include <library/cpp/text_processing/tokenizer/lemmer_impl.h>

#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/core/lemmer.h>

namespace {
    TLangMask ConvertLanguagesToMask(const TVector<ELanguage>& languages) {
        if (languages.empty()) {
            return NLanguageMasks::AllLanguages();
        }

        TLangMask result;
        for (auto language : languages) {
            result |= TLangMask(language);
        }
        return result;
    }

    class TYandexSpecificLemmerImplementation : public NTextProcessing::NTokenizer::ILemmerImplementation {
    public:
        TYandexSpecificLemmerImplementation(const TVector<ELanguage> languages)
            : Languages(ConvertLanguagesToMask(languages))
        {
        }

        void Lemmatize(TUtf16String* token) const override {
            TWLemmaArray lemmas;
            NLemmer::AnalyzeWord(token->data(), token->size(), lemmas, Languages);
            for (const auto& lemma : lemmas) {
                if (lemma.GetLanguage() != LANG_UNK) {
                    *token = lemma.GetText();
                    return;
                }
            }
        }

    private:
        TLangMask Languages;
    };
}

NTextProcessing::NTokenizer::TLemmerImplementationFactory::TRegistrator<TYandexSpecificLemmerImplementation> YandexSpecificLemmerImplementationRegistrator(NTextProcessing::NTokenizer::EImplementationType::YandexSpecific);
