#pragma once

#include "calculator_config.h"
#include <kernel/dssm_applier/nn_applier/lib/layers.h>
#include <kernel/ethos/lib/text_classifier/binary_classifier.h>
#include <kernel/facts/dist_between_words/trie_data.h>
#include <kernel/facts/word_embedding/word_embedding.h>
#include <kernel/normalize_by_lemmas/normalize.h>
#include <ml/neocortex/neocortex_lib/classifiers.h>
#include <ysite/yandex/pure/pure.h>

namespace NUnstructuredFeatures {
    struct TCalculatorData {
        TCalculatorData(const TConfig& config);
        NNeuralNetApplier::TModelPtr RuFactSnippetDssmModel;
        NNeuralNetApplier::TModelPtr TomatoDssmModel;
        NNeuralNetApplier::TModelPtr AliasesBdssmModel;
        THolder< TNormalizeByLemmasInfo> Normalizer;
        THolder<NNeocortex::TTextClassifierPack> TextToTextNeocortexModel;
        THolder<NNeocortex::TTextClassifierPack> TextToHostsNeocortexModel;
        THolder<NNeocortex::TTextClassifierPack> LemmaAliasNeocortexModel;
        THolder<NNeocortex::TTextClassifierPack> TextToTextFactSnipNeocortexModel;
        THolder<NNeocortex::TTextClassifierPack> SerpItemsNeocortexModel;
        NDistBetweenWords::TWordDistTriePtr WordsDistTrie;
        NDistBetweenWords::TWordDistTriePtr BigramsDistTrie;
        THolder<NWordEmbedding::TWordEmbeddingFeaturesBuilder> WordEmbeddingFeaturesBuilder;
        THolder<TPure> Pure;
        THolder<NEthos::TBinaryTextClassifierModel> QueryUnigramModel;
        THolder<NEthos::TBinaryTextClassifierModel> CrossModel;
    };

    template <typename T> void EnsureData(const T &ptr, TStringBuf name) {
        if (!ptr)
            ythrow yexception() << name << " wasn't loaded";
    }
    #define ENSURE_DATA(data, field) EnsureData(data.field, #field)
}
