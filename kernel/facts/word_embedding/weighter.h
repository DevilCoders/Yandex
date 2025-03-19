#pragma once

#include <kernel/facts/features_calculator/analyzed_word/analyzed_word.h>

#include <library/cpp/langs/langs.h>
#include <quality/trailer/trailer_common/mmap_preloaded/mmap_loader.h>
#include <ysite/yandex/pure/pure.h>

#include <util/charset/wide.h>
#include <util/generic/vector.h>

namespace NUnstructuredFeatures {
    namespace NWordEmbedding {
        class TWeighter {
        public:
            virtual ~TWeighter() {}
            virtual void Weight(const TVector<TAnalyzedWord>& input, TVector<float>& weights) const = 0;
        };

        // Smooth Inverse Frequency [alpha / (alpha + frequency(word))]
        class TSIFWeighter : public TWeighter {
        public:
            TSIFWeighter(float alpha)
                : Alpha(alpha) {}
            void Weight(const TVector<TAnalyzedWord>& input, TVector<float>& weights) const override;
        private:
            float Alpha;
        };
    }
}
