#include "preprocessor.h"


#include <library/cpp/dot_product/dot_product.h>
#include <kernel/lemmer/core/language.h>

#include <util/generic/hash.h>
#include <util/generic/hash_set.h>


namespace NUnstructuredFeatures {
    namespace NWordEmbedding {

        void TSubtractLemmasPreprocessor::Do(THashMap<TString, TVector<TAnalyzedWord>>& data) const {
            const TVector<TAnalyzedWord>& leftInput = data[LeftInputAnnotation];
            const TVector<TAnalyzedWord>& rightInput = data[RightInputAnnotation];
            TVector<TAnalyzedWord>& output = data[OutputAnnotation];

            TVector<TUtf16String> leftLemmas;
            TVector<TUtf16String> rightLemmas;

            GetTopLemmas(leftInput, leftLemmas);
            GetTopLemmas(rightInput, rightLemmas);

            THashSet<TUtf16String> rightLemmasSet(rightLemmas.begin(), rightLemmas.end());
            for (size_t i = 0; i < leftLemmas.size(); i++) {
                if (!rightLemmasSet.contains(leftLemmas[i])) {
                    output.push_back(leftInput[i]);
                }
            }
        }

        void TSubtractLemmasPreprocessor::GetTopLemmas(const TVector<TAnalyzedWord>& input, TVector<TUtf16String>& lemmas) const {
            lemmas.reserve(input.size());
            for (const TAnalyzedWord& analyzedWord : input) {
                if (!analyzedWord.Lemmas.empty()) {
                    const TYandexLemma& yLemma = analyzedWord.Lemmas.front();
                    const TUtf16String& lemma = TUtf16String(yLemma.GetText(), yLemma.GetTextLength());
                    lemmas.push_back(lemma);
                } else {
                    lemmas.push_back(analyzedWord.Word);
                }
            }
        }
    }
}
