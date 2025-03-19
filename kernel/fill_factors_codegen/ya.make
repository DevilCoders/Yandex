PROGRAM()

OWNER(
    g:base
    noobgam
)

SRCS(
    main.cpp
)

PEERDIR(
    kernel/factor_slices

    kernel/fill_factors_codegen/proto

    library/cpp/getopt
    library/cpp/protobuf/util
    search/meta/factors

# if you need extra EFactorSlice, adding peerdir here would be enough
    kernel/web_factors_info
    kernel/web_meta_factors_info
    kernel/web_new_l1_factors_info
    kernel/web_itditp_factors_info
    kernel/web_meta_itditp_factors_info
    kernel/begemot_model_factors_info
    kernel/begemot_query_factors_info
    kernel/begemot_query_rt_factors_info
    kernel/begemot_query_rt_l2_factors_info
    kernel/itditp_user_history_factors_info
    kernel/neural_network_over_dssm_factors_info
    kernel/images_nn_over_dssm_doc_features/factors
    kernel/web_itditp_factors_info
    kernel/web_itditp_static_features_factors_info
    kernel/web_itditp_recommender_factors_info
    kernel/alice/begemot_nlu_factors_info
    kernel/alice/video_scenario/video_scenario_factors_info
    kernel/alice/music_scenario_factors_info
    kernel/alice/search_scenario_factors_info
    kernel/alice/direct_scenario_factors_info
    kernel/alice/gc_scenario_factors_info
    kernel/rapid_clicks_l2_factors_info
    extsearch/images/base/newl1factors
    extsearch/video/kernel/factors_info
)

END()
