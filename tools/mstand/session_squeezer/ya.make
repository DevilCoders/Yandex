OWNER(
    g:mstand-squeeze
)

PY3_LIBRARY()

PEERDIR(
    metrika/uatraits/python
    quality/functionality/turbo/urls_lib/python/lib
    quality/yaqlib/yaqutils
    scarab/api/python3

    tools/mstand/experiment_pool
    tools/mstand/mstand_enums
    tools/mstand/mstand_utils
    tools/mstand/mstand_structs
    tools/mstand/user_plugins
)

PY_SRCS(
    NAMESPACE session_squeezer
    __init__.py
    distribution_kludge.py
    experiment_for_squeeze.py
    referer.py
    services.py
    squeeze_runner.py
    squeezer.py
    squeezer_alice.py
    squeezer_app_metrics_toloka.py
    squeezer_common.py
    squeezer_cv.py
    squeezer_ecom.py
    squeezer_ether.py
    squeezer_images.py
    squeezer_intrasearch.py
    squeezer_intrasearch_metrika.py
    squeezer_market_search_sessions.py
    squeezer_market_sessions_stat.py
    squeezer_market_web_reqid.py
    squeezer_morda.py
    squeezer_news.py
    squeezer_object_answer.py
    squeezer_ott_impressions.py
    squeezer_ott_sessions.py
    squeezer_pp.py
    squeezer_prism.py
    squeezer_recommender.py
    squeezer_surveys.py
    squeezer_toloka.py
    squeezer_turbo.py
    squeezer_vh.py
    squeezer_video.py
    squeezer_watchlog.py
    squeezer_web.py
    squeezer_web_cache.py
    squeezer_web_extended.py
    squeezer_web_surveys.py
    squeezer_ya_metrics.py
    squeezer_ya_video.py
    squeezer_yql_ab.py
    squeezer_yuid_reqid_testid_filter.py
    squeezer_zen.py
    squeezer_zen_surveys.py
    suggest.py
    validation.py
)

END()

RECURSE_FOR_TESTS(
    tests
)
