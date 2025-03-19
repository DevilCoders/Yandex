#pragma once

#include <kernel/facts/features_calculator/embeddings/embeddings.h>

#include <util/memory/blob.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>


namespace NUnstructuredFeatures {
    namespace NWordEmbedding {
        class TDictionary {
        public:
            TDictionary(const TString& wordsFileName, const TString& vectorsFileName);
            bool GetEmbedding(const TUtf16String& word, TEmbedding& embedding) const;
            size_t GetSize() const;
            size_t GetEmbeddingSize() const;
        private:
            TBlob Embeddings;
            THashMap<TUtf16String, size_t> WordToIndex;
            size_t Size;
            size_t EmbeddingSize;
        };
    }
}
