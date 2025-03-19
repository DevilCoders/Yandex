#pragma once

namespace NDoom {


enum EStandardIoModel {
    NoStandardIoModel    = 0,

    PantherKeyIoModelV1                                                 /* "panther_key_io_model_v1" */,
    PantherHitIoModelV1                                                 /* "panther_hit_io_model_v1" */,

    AttributesKeyIoModelV1                                              /* "attributes_key_io_model_v1" */,
    AttributesHitIoModelV1                                              /* "attributes_hit_io_model_v1" */,

    DoublePantherKeyIoModelV1                                           /* "double_panther_key_io_model_v1" */,
    DoublePantherHitIoModelV1                                           /* "double_panther_hit_io_model_v1" */,

    AnnDataHitIoModelV1                                                 /* "ann_data_hit_io_model_v1" */,

    AnnKeyIoModelV1                                                     /* "ann_key_io_model_v1" */,
    AnnHitIoModelV1                                                     /* "ann_hit_io_model_v1" */,

    SentHitIoModelV1                                                    /* "sent_hit_io_model_v1" */,
    AnnSentHitIoModelV1                                                 /* "ann_sent_hit_io_model_v1" */,
    FactorAnnSentHitIoModelV1                                           /* "factor_ann_sent_hit_io_model_v1" */,
    LinkAnnSentHitIoModelV1                                             /* "link_ann_sent_hit_io_model_v1" */,

    AnnSortedMultiKeysKeyIoModelV1                                      /* "ann_sorted_multi_keys_key_io_model_v1" */,
    AnnSortedMultiKeysHitIoModelV1                                      /* "ann_sorted_multi_keys_hit_io_model_v1" */,

    CountsKeyIoModelV1                                                  /* "counts_key_io_model_v1" */,
    CountsHitIoModelV1                                                  /* "counts_hit_io_model_v1" */,

    PortionCountsKeyIoModelV1                                           /* "portion_counts_key_io_model_v1" */,
    PortionCountsHitIoModelV1                                           /* "portion_counts_hit_io_model_v1" */,

    PortionAttributesKeyIoModelV1                                       /* "portion_attributes_key_io_model_v1" */,
    PortionAttributesHitIoModelV1                                       /* "portion_attributes_hit_io_model_v1" */,

    PortionPantherKeyIoModelV1                                          /* "portion_panther_key_io_model_v1" */,
    PortionPantherHitIoModelV1                                          /* "portion_panther_hit_io_model_v1" */,

    ClickSimIoModel                                                     /* "click_sim_io_model_v1" */,

    DocPortionKeyIoModelV1                                              /* "doc_portion_key_io_model_v1" */,
    DocPortionHitIoModelV1                                              /* "doc_portion_hit_io_model_v1" */,

    OmniUrlIoModelV1                                                    /* "omni_url_io_model_v1" */,
    OmniTitleIoModelV1                                                  /* "omni_title_io_model_v1" */,
    OmniDssmLogDwellTimeBigramsEmbeddingIoModelV1                       /* "omni_dssm_log_dwell_time_bigrams_embedding_io_model_v1" */,
    OmniAnnRegStatsIoModelV1                                            /* "omni_ann_reg_stats_io_model_v1" */,
    OmniDssmAggregatedAnnRegEmbeddingIoModelV1                          /* "omni_dssm_aggregated_ann_reg_embedding_io_model_v1" */,
    OmniDssmAnnCtrCompressedEmbeddingIoModelV1                          /* "omni_dssm_ann_ctr_compressed_embedding_io_model_v1" */,
    OmniDssmAnnXfDtShowWeightCompressedEmbedding1IoModelV1              /* "omni_dssm_ann_xf_dt_show_weight_compressed_embedding1_io_model_v1" */,
    OmniDssmAnnXfDtShowWeightCompressedEmbedding2IoModelV1              /* "omni_dssm_ann_xf_dt_show_weight_compressed_embedding2_io_model_v1" */,
    OmniDssmAnnXfDtShowWeightCompressedEmbedding3IoModelV1              /* "omni_dssm_ann_xf_dt_show_weight_compressed_embedding3_io_model_v1" */,
    OmniDssmAnnXfDtShowWeightCompressedEmbedding4IoModelV1              /* "omni_dssm_ann_xf_dt_show_weight_compressed_embedding4_io_model_v1" */,
    OmniDssmAnnXfDtShowWeightCompressedEmbedding5IoModelV1              /* "omni_dssm_ann_xf_dt_show_weight_compressed_embedding5_io_model_v1" */,
    OmniDssmAnnXfDtShowOneCompressedEmbeddingIoModelV1                  /* "omni_dssm_ann_xf_dt_show_one_compressed_embedding_io_model_v1" */,
    OmniDssmAnnXfDtShowOneSeCompressedEmbeddingIoModelV1                /* "omni_dssm_ann_xf_dt_show_one_se_compressed_embedding_io_model_v1" */,
    OmniDssmMainContentKeywordsEmbeddingIoModelV1                       /* "omni_dssm_main_content_keywords_embedding_io_model_v1" */,

    ErfIoModelV1                                                        /* "erf_io_model_v1" */,
    RegHostErfIoModelV1                                                 /* "regherf_io_model_v1" */,
    HostErfIoModelV1                                                    /* "host_erf_io_model_v1" */,
    RegErfIoModelv1                                                     /* "regerf_io_model_v1" */,

    DocAttrsHitIoModelV1                                                /* "doc_attrs_hit_io_model_v1" */,
    DocAttrs64HitIoModelV1                                              /* "doc_attrs64_hit_io_model_v1" */,

    NameToCategHostModelV1                                              /* "name_to_categ_host_model_v1" */,
    NameToCategDomainModelV1                                            /* "name_to_categ_domain_model_v1" */,

    PantherNgramsHashModelV1                                            /* "panther_ngrams_hash_model_v1" */,
    PantherNgramsHitModelV1                                             /* "panther_ngrams_hit_model_v1" */,

    ImagesAnnSortedMultiKeysKeyIoModelV1                                /* "img_ann_sorted_multi_keys_key_io_model_v1" */,
    ImagesAnnSortedMultiKeysHitIoModelV1                                /* "img_ann_sorted_multi_keys_hit_io_model_v1" */,

    InvertedIndexPantherHashModelV1                                     /* "inverted_index_panther_hash_model_v1" */,
    InvertedIndexPantherHitModelV1                                      /* "inverted_index_panther_hit_model_v1" */,

    KeyInvLemmasRemappingsIoModelV1                                     /* "keyinv_lemmas_remappings_io_model_v1" */,
    KeyInvFormsRemappingsIoModelV1                                      /* "keyinv_forms_remappings_io_model_v1" */,
    KeyInvHitIoModelV1                                                  /* "keyinv_hit_io_model_v1" */,
    KeyInvHitIoModelV2                                                  /* "keyinv_hit_io_model_v2" */,

    ErasureLocationsModelV1                                             /* "erasure_locations_model_v1" */,

    DefaultOmniUrlIoModel = OmniUrlIoModelV1,
    DefaultOmniTitleIoModel = OmniTitleIoModelV1,
    DefaultOmniDssmLogDwellTimeBigramsEmbeddingIoModel = OmniDssmLogDwellTimeBigramsEmbeddingIoModelV1,
    DefaultOmniAnnRegStatsIoModel = OmniAnnRegStatsIoModelV1,
    DefaultOmniDssmAggregatedAnnRegEmbeddingIoModel = OmniDssmAggregatedAnnRegEmbeddingIoModelV1,
    DefaultOmniDssmAnnCtrCompressedEmbeddingIoModel = OmniDssmAnnCtrCompressedEmbeddingIoModelV1,
    DefaultOmniDssmAnnXfDtShowWeightCompressedEmbedding1IoModel = OmniDssmAnnXfDtShowWeightCompressedEmbedding1IoModelV1,
    DefaultOmniDssmAnnXfDtShowWeightCompressedEmbedding2IoModel = OmniDssmAnnXfDtShowWeightCompressedEmbedding2IoModelV1,
    DefaultOmniDssmAnnXfDtShowWeightCompressedEmbedding3IoModel = OmniDssmAnnXfDtShowWeightCompressedEmbedding3IoModelV1,
    DefaultOmniDssmAnnXfDtShowWeightCompressedEmbedding4IoModel = OmniDssmAnnXfDtShowWeightCompressedEmbedding4IoModelV1,
    DefaultOmniDssmAnnXfDtShowWeightCompressedEmbedding5IoModel = OmniDssmAnnXfDtShowWeightCompressedEmbedding5IoModelV1,
    DefaultOmniDssmAnnXfDtShowOneCompressedEmbeddingIoModel = OmniDssmAnnXfDtShowOneCompressedEmbeddingIoModelV1,
    DefaultOmniDssmAnnXfDtShowOneSeCompressedEmbeddingIoModel = OmniDssmAnnXfDtShowOneSeCompressedEmbeddingIoModelV1,
    DefaultOmniDssmMainContentKeywordsEmbeddingIoModel = OmniDssmMainContentKeywordsEmbeddingIoModelV1,

    DefaultErfIoModel = ErfIoModelV1,

    DefaultPantherKeyIoModel = PantherKeyIoModelV1,
    DefaultPantherHitIoModel = PantherHitIoModelV1,

    DefaultAttributesKeyIoModel = AttributesKeyIoModelV1,
    DefaultAttributesHitIoModel = AttributesHitIoModelV1,

    DefaultDoublePantherKeyIoModel = DoublePantherKeyIoModelV1,
    DefaultDoublePantherHitIoModel = DoublePantherHitIoModelV1,

    DefaultAnnDataHitIoModel = AnnDataHitIoModelV1,

    DefaultAnnKeyIoModel = AnnKeyIoModelV1,
    DefaultAnnHitIoModel = AnnHitIoModelV1,

    DefaultAnnSortedMultiKeysKeyIoModel = AnnSortedMultiKeysKeyIoModelV1,
    DefaultAnnSortedMultiKeysHitIoModel = AnnSortedMultiKeysHitIoModelV1,

    DefaultKeyInvHitIoModel = KeyInvHitIoModelV1,

    DefaultCountsKeyIoModel = CountsKeyIoModelV1,
    DefaultCountsHitIoModel = CountsHitIoModelV1,

    DefaultDocPortionKeyIoModel = DocPortionKeyIoModelV1,
    DefaultDocPortionHitIoModel = DocPortionHitIoModelV1,

    DefaultRegHostErfIoModel = RegHostErfIoModelV1,
    DefaultHostErfIoModel = HostErfIoModelV1,
    DefaultRegErfIoModel = RegErfIoModelv1,

    DefaultSentIoModel = SentHitIoModelV1,
    DefaultAnnSentIoModel = AnnSentHitIoModelV1,
    DefaultFactorAnnSentIoModel = FactorAnnSentHitIoModelV1,
    DefaultLinkAnnSentIoModel = LinkAnnSentHitIoModelV1,

    DefaultDocAttrsHitIoModel = DocAttrsHitIoModelV1,
    DefaultDocAttrs64HitIoModel = DocAttrs64HitIoModelV1,

    DefaultPantherNgramsHashModel = PantherNgramsHashModelV1,
    DefaultPantherNgramsHitModel = PantherNgramsHitModelV1,

    DefaultImagesAnnSortedMultiKeysKeyIoModel = ImagesAnnSortedMultiKeysKeyIoModelV1,
    DefaultImagesAnnSortedMultiKeysHitIoModel = ImagesAnnSortedMultiKeysHitIoModelV1,

    DefaultInvertedIndexPantherHashModel = InvertedIndexPantherHashModelV1,
    DefaultInvertedIndexPantherHitModel = InvertedIndexPantherHitModelV1,

    DefaultErasureLocationsModel = ErasureLocationsModelV1,
};


} // namespace NDoom
