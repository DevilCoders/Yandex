OWNER(
    ilnurkh
    g:factordev
)

LIBRARY()

PEERDIR(
    #factors-info
    kernel/alice/asr_factors_info
    kernel/alice/begemot_nlu_factors_info
    kernel/alice/begemot_query_factors_info
    kernel/alice/device_state_factors_info
    kernel/alice/direct_scenario_factors_info
    kernel/alice/gc_scenario_factors_info
    kernel/alice/music_scenario_factors_info
    kernel/alice/query_tokens_factors_info
    kernel/alice/search_scenario_factors_info
    kernel/alice/video_scenario/video_scenario_factors_info
    search/web/blender/factors_info

    kernel/begemot_model_factors_info
    kernel/begemot_query_factors_info
    kernel/begemot_query_rt_factors_info
    kernel/begemot_query_rt_l2_factors_info
    kernel/rapid_clicks_l2_factors_info
    search/web_fresh_detector

    kernel/itditp_user_history_factors_info
    kernel/web_itditp_factors_info
    kernel/web_meta_itditp_factors_info
    kernel/web_itditp_static_features_factors_info
    kernel/web_itditp_recommender_factors_info
    kernel/web_discovery_factors_info

    kernel/web_factors_info
    kernel/web_new_l1_factors_info
    kernel/web_l1_factors_info
    kernel/web_l2_factors_info
    kernel/web_l3_mxnet_factors_info
    kernel/web_meta_factors_info
    kernel/web_meta_pers_factors_info
    kernel/web_production_formula_features_factors_info
    kernel/web_rtmodels_factors_info

    kernel/robot_selectionrank_factors_info
    kernel/neural_network_over_dssm_factors_info
    kernel/snippets/factors

    extsearch/images/base/factors
    extsearch/images/base/newl1factors
    extsearch/video/kernel/factors_info

    search/personalization
    search/rapid_clicks
    search/l4_features
)

END()
