#pragma once

namespace NDoom {

/**
 * Semantic type of the index that is part of a WAD.
 *
 * Answers to the questions "what this index is used for".
 *
 * Please don't include separate values for:
 * 1. Different index formats. Format description should go into info lump.
 * 2. Different search verticals. E.g. we shouldn't have `ImagesErf` and `WebErf`.
 */
enum EWadIndexType {
    PantherIndexType                    /* "panther" */,
    AttributesIndexType                 /* "attributes" */,
    FactorAnnDataIndexType              /* "factor_ann_data" */,
    FactorAnnIndexType                  /* "factor_ann_freq_sorted" */, // TODO: DROP! Includes format!
    FastAnnDataIndexType                /* "fast_ann_data" */,
    FastAnnIndexType                    /* "fast_ann" */,
    FactorSentIndexType                 /* "factor_sent" */,
    FastSentIndexType                   /* "fast_sent" */,

    AnnSentIndexType                    /* "ann_sent" */,
    FactorAnnSentIndexType              /* "factor_ann_sent" */,
    LinkAnnSentIndexType                /* "link_ann_sent" */,
    SentIndexType                       /* "sent" */,

    DocAttrsIndexType                   /* "doc_attrs" */,
    CategToNameIndexType                /* "categ_to_name" */,
    NameToCategIndexType                /* "name_to_categ" */,
    ArcIndexType                        /* "arc" */,

    ErfIndexType                        /* "erf" */,
    HostErfIndexType                    /* "host_erf" */,
    //strange erf naming due wrong initioal choice
    RegErfIndexType                     /* "reg_erf" */,
    RegHostErfIndexType                 /* "reg_host_erf" */,
    DocFreqsIndexType                   /* "doc_frq" */,
    InvUrlHashesIndexType               /* "inv_url_hashes" */,
    ExtInfoArcIndexType                 /* "ext_info_arc" */,
    LinkAnnArcIndexType                 /* "link_ann_arc" */,
    // Will be deleted {
    OmniIndexType                       /* "omni" */,
    OmniVideoIndexType                  /* "omni_video" */, // TODO: why separate index type? This one should serve the same role as OmniIndexType! DROP!
    // }
    OmniUrlType                         /* "omni_url" */,
    OmniDupUrlsType                     /* "omni_dup_urls" */,
    OmniEmbedUrlsType                   /* "omni_embed_urls" */,
    OmniTitleType                       /* "omni_title" */,
    OmniDssmEmbeddingType               /* "omni_dssm_embedding" */,
    OmniDssmWeightedAnnotationType      /* "omni_dssm_weighted_annotation" */, // deprecated
    OmniAnnRegStatsType                 /* "omni_ann_reg_stats" */,
    OmniDssmConcatenatedAnnRegEmbeddingType /* "omni_dssm_concatenated_ann_reg_embedding" */, // deprecated
    OmniDssmAggregatedAnnRegEmbeddingType   /* "omni_dssm_aggregated_ann_reg_embedding" */,
    OmniDssmAnnXfDtShowOneEmbeddingsType    /* "omni_dssm_ann_xf_dt_show_one_embedding" */, // deprecated
    OmniDssmAnnXfDtShowWeightEmbeddingsType /* "omni_dssm_ann_xf_dt_show_weight_embedding" */, // deprecated
    OmniDssmAnnCtrCompressedEmbeddingType               /* "omni_dssm_ann_ctr_compressed_embedding" */,
    OmniDssmAnnXfDtShowWeightCompressedEmbedding1Type   /* "omni_dssm_ann_xf_dt_show_weight_compressed_embedding1" */,
    OmniDssmAnnXfDtShowWeightCompressedEmbedding2Type   /* "omni_dssm_ann_xf_dt_show_weight_compressed_embedding2" */,
    OmniDssmAnnXfDtShowWeightCompressedEmbedding3Type   /* "omni_dssm_ann_xf_dt_show_weight_compressed_embedding3" */,
    OmniDssmAnnXfDtShowWeightCompressedEmbedding4Type   /* "omni_dssm_ann_xf_dt_show_weight_compressed_embedding4" */,
    OmniDssmAnnXfDtShowWeightCompressedEmbedding5Type   /* "omni_dssm_ann_xf_dt_show_weight_compressed_embedding5" */,
    OmniDssmAnnXfDtShowWeightCompressedEmbeddingsType   /* "omni_dssm_ann_xf_dt_show_weight_compressed_embeddings" */,
    OmniDssmAnnXfDtShowOneCompressedEmbeddingType       /* "omni_dssm_ann_xf_dt_show_one_compressed_embedding" */,
    OmniDssmAnnXfDtShowOneSeCompressedEmbeddingType     /* "omni_dssm_ann_xf_dt_show_one_se_compressed_embedding" */,
    OmniDssmAnnXfDtShowOneSeAmSsHardCompressedEmbeddingType     /* "omni_dssm_ann_xf_dt_show_one_se_am_ss_hard_compressed_embedding" */,
    OmniAliceWebMusicTrackTitleType                             /* "omni_alice_web_music_track_title" */,
    OmniAliceWebMusicArtistNameType                             /* "omni_alice_web_music_artist_name" */,
    OmniDssmAnnSerpSimilarityHardSeCompressedEmbeddingType     /* "omni_dssm_ann_serp_similarity_hard_se_compressed_embedding" */,
    OmniDssmMainContentKeywordsEmbeddingType            /* "omni_dssm_main_content_keywords_embedding" */,
    OmniDssmPantherTermsEmbeddingType   /* omni_dssm_panther_terms_embedding_type */,
    ImgLinkDataAnnDataIndexType         /* "img_link_data_ann_data" */, // TODO: why Img prefix? DROP!
    ImgLinkDataSentIndexType            /* "img_link_data_sent" */, // TODO: why Img prefix? DROP!
    OmniRecDssmSpyTitleDomainCompressedEmb12Type      /* "omni_rec_dssm_spy_title_domain_compressed_emb12_type" */,
    OmniRecCFSharpDomainType            /* "omni_rec_cf_sharp_domain_type" */,
    OmniDssmFpsSpylogAggregatedEmbeddingType /* "omni_dssm_fps_spylog_aggregated_embedding_type" */,
    OmniDssmUserHistoryHostClusterEmbeddingType /* "omni_dssm_user_history_host_cluster_embedding_type" */,
    OmniRelCanonicalTargetType  /* "omni_rel_canonical_target_type" */,

    ChunkMappingIndexType               /* "chunk_mapping" */,
    ClickSimQuerySideIndexType          /* "click_sim_query_side" */,

    OmniVideoVpqEmbeddingType           /* "omni_video_vpq_embedding" */,
    OmniVideoUserNameType               /* "omni_video_user_name" */,
    OmniVideoObjectType                 /* "omni_video_object" */,
    OmniUserFactorsType                 /* "omni_user_factors" */,
    OmniRecDssmEmbeddingType            /* "omni_rec_dssm_embedding" */,
    OmniLivePropertiesType              /* "omni_live_properties" */,
    OmniVideoVpqV2EmbeddingType         /* "omni_video_vpq_v2_embedding" */,
    OmniVideohubCategoriesType          /* "omni_videohub_categories" */,
    OmniVideoAllowedRegionsType         /* "omni_video_allowed_regions" */,
    OmniVideoVpqV3EmbeddingType         /* "omni_video_vpq_v3_embedding" */,

    AssessmentBasePerQueryInfo          /*  "asssesment_base_query_info" */,
    AssessmentBasePerUrlInfo            /*  "asssesment_base_url_info" */,

    PantherNgramsIndexType              /* "panther_ngrams" */,

    OmniVideoPersonalProfileType        /* "omni_video_personal_profile" */,
    OmniVideoLicenseType                /* "omni_video_license" */,

    ThesaurusExtenType                  /* "thes_exten_type" */,
    ThesaurusExtenTextType              /* "thes_exten_text_type" */,

    ErasurePartLocations                /* "erasure_part_locations" */,

    OmniVideoEntityProfilesType         /* "omni_video_entity_profiles" */,

    OmniRootUuidType                    /* "omni_root_uuid" */,
    SimilarHostsType                    /* "similar_hosts" */,
    OmniVhUuidType                      /* "omni_vh_uuid" */,

    HnswIndexPerFormulaType             /* "hnsw_index_per_formula" */,
    HnswDocsMappingPerFormulaType       /* "hnsw_docs_mapping_per_formula" */,

    WebItdItpStaticFeaturesIndexType    /* "web_itditp_static_features" */,

    OmniVideoDocPopularityType          /* "omni_video_doc_popularity" */,

    OmniVideoRestrictionAgeType         /* "omni_video_restriction_age" */,
    OmniVideoWebBertRelevType           /* "omni_video_web_bert_relev" */,

    BertEmbeddingIndexType              /* "bert_embedding" */,
    BertEmbeddingV2IndexType            /* "bert_embedding_v2" */,

    CheckSumIndexType                   /* "check_sum" */,

    OmniVideoAutoplaySafety             /* "video_autoplay_safety" */,

    KeyInvIndexType                     /* "key_inv" */,

    RealTimeTrainingDictProviderType    /* "real_time_training_dict_provider" */,

    EmbeddingStorageAttributesType      /* "embedding_storage_attributes" */,
    EmbeddingStorageAttributesCapacityType /* "embedding_storage_attributes_capacity" */,

    OmniDssmBertDistillL2EmbeddingType  /* "omni_dssm_bert_distill_l2_embedding" */,

    AnnIndexType                        /* "ann" */,
    LinkAnnIndexType                    /* "link_ann" */,

    AnnDataIndexType                    /* "ann_data" */,
    LinkAnnDataIndexType                /* "link_ann_data" */,

    WebItdItpSlimIndexType              /* "web_itditp_slim_index" */,

    ErasurePartLocationsOffsetOnly      /* "erasure_part_locations_offset_only" */,
    ErasureDocInPartLocation            /* "erasure_doc_in_part_location" */,

    OmniVideoVpqV4EmbeddingType         /* "omni_video_vpq_v4_embedding" */,
    ItemStorage                         /* "item_storage" */,
    Speech2TextIndexType                /* "speech2text" */,

    ImagesCommercialOffersIndexType     /* "images_commercial_offers" */,

    Speech2TextDssmIndexType            /* "speech2textDssm" */,

    OmniVideoClickSimIndexType          /* "omni_video_click_simularity" */,

    OmniDssmNavigationL2EmbeddingType  /* "omni_dssm_navigation_l2_embedding" */,

    OmniVideoVpqV5EmbeddingType         /* "omni_video_vpq_v5_embedding" */,

    OmniDssmFullSplitBertEmbeddingType           /* "omni_dssm_full_split_bert_embedding" */,

    ImagesCommercialOffersRegionsType   /* "images_commercial_offers_regions" */,

    OmniMarketClickSimIndexType         /* "omni_market_click_similarity" */,
    OmniMarketSuperEmbedDssmIndexType   /* "omni_market_super_embed_dssm" */,
    OmniMarketBmCatsIndexType           /* "omni_market_bm_categories" */,

    OmniVideoLiteRvbProfileType         /* "omni_video_lite_rvb_profile" */,

    OmniDssmSinsigL2EmbeddingType  /* "omni_dssm_sinsig_l2_embedding" */,
    OmniReservedEmbeddingType           /* "omni_reserved_embedding" */,  // revert temporarily during Jupiter migration
    CbirGoodsStaticData                     /* cbirGoodsStaticData */,
};

} // namespace NDoom
