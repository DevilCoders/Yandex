#pragma once
#include <kernel/embeddings_info/embedding.h>
#include <kernel/embeddings_info/embedding.h_serialized.h>
#include <util/generic/cast.h>
#include <util/generic/string.h>
#include <array>

namespace NDssmApplier {
    struct TDocEmbeddingTransferTraits {
        size_t RequiredEmbeddingSize;
        EDssmDocEmbeddingType Type;
        TString DssmEmbeddingLayer;

        TDocEmbeddingTransferTraits(size_t size, EDssmDocEmbeddingType type, const TString& layer)
            : RequiredEmbeddingSize(size)
            , Type(type)
            , DssmEmbeddingLayer(layer)
        {}
    };

    const std::array<TDocEmbeddingTransferTraits, GetEnumItemsCount<EDssmDocEmbedding>()> DocEmbeddingsTransferTraits = {{
        {300, EDssmDocEmbeddingType::Uncompressed, "doc_embedding_bigrams"},
        {25, EDssmDocEmbeddingType::Compressed, "doc_embedding_log_dt_bigrams_am_hard_queries_no_clicks"},
        {40, EDssmDocEmbeddingType::CompressedWithWeights, "doc_embedding_dssm_boosting_xfdtshow_onese_amss_hard"},
        {30, EDssmDocEmbeddingType::Compressed, "doc_embedding_dwelltime_reg_chain_embedding"},
        {100, EDssmDocEmbeddingType::Compressed, "doc_embedding_query_word_title_embedding"},
        {50, EDssmDocEmbeddingType::CompressedLogDwellTimeL2, ""},
        {72, EDssmDocEmbeddingType::CompressedPantherTerms, ""},
        {400, EDssmDocEmbeddingType::CompressedUserRecDssmSpyTitleDomain, ""},
        {400, EDssmDocEmbeddingType::CompressedUrlRecDssmSpyTitleDomain, ""},
        {18, EDssmDocEmbeddingType::UncompressedCFSharp, ""},
        {120, EDssmDocEmbeddingType::Compressed, "doc_embedding_reformulations_longest_click_log_dt"},
        {120, EDssmDocEmbeddingType::Compressed, "doc_embedding_dssm_bert_distill_sinsig_mse_base_reg_chain"},
        {120, EDssmDocEmbeddingType::Compressed, "doc_embedding_dssm_bert_distill_relevance_mse_base_reg_chain"},
    }};
}
