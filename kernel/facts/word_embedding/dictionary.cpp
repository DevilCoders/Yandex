#include "dictionary.h"

#include <kernel/dssm_applier/utils/utils.h>
#include <quality/trailer/trailer_common/mmap_preloaded/mmap_loader.h>

#include <util/stream/file.h>

namespace NUnstructuredFeatures {
    namespace NWordEmbedding {
        TDictionary::TDictionary(const TString& wordsFileName, const TString& vectorsFileName) {
            TFileInput wordsFile(wordsFileName);
            wordsFile >> Size >> EmbeddingSize;
            TUtf16String line;
            while (wordsFile.ReadLine(line)) {
                size_t index = WordToIndex.size();
                WordToIndex[line] = index;
            }
            Embeddings = SuggestBlobFromFile(vectorsFileName);
            Y_ENSURE(Embeddings.Size() == Size * EmbeddingSize * sizeof(ui8));
        }

        bool TDictionary::GetEmbedding(const TUtf16String& word, TEmbedding& embedding) const {
            const size_t* index = WordToIndex.FindPtr(word);
            if (index == nullptr) {
                embedding.Assign(TVector<float>(EmbeddingSize, 0.0f));
                return false;
            }
            const ui8* begin = static_cast<const ui8*>(Embeddings.Data()) + *index * EmbeddingSize;
            embedding.Assign(NDssmApplier::NUtils::TFloat2UI8Compressor::Decompress(TArrayRef<const ui8>(begin, EmbeddingSize)));
            return true;
        }

        size_t TDictionary::GetSize() const {
            return Size;
        }

        size_t TDictionary::GetEmbeddingSize() const {
            return EmbeddingSize;
        }
    }
}
