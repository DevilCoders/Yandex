PY3_LIBRARY()

OWNER(
    g:mstand
)

PY_SRCS(
    NAMESPACE sample_metrics.offline
    __init__.py
    five_cg_detailed.py
    five_cg_official.py
    five_cg.py
    four_cg_detailed.py
    get_prod_metric.py
    images_composite.py
    images_test_metric.py
    judgement_level.py
    national_domais_metric.py
    offline_test_metrics.py
    pfound.py
    serp_extend_test_metric.py
    video_pfound_mc.py
    video_pfound.py
)

PEERDIR(
    quality/yaqlib/yaqutils
    quality/yaqlib/yaqlibenums
    tools/mstand/experiment_pool
    tools/mstand/serp
)

END()
