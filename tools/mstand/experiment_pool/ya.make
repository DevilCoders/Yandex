OWNER(
    g:mstand
)

PY3_LIBRARY()

PEERDIR(
    contrib/python/attrs
    quality/yaqlib/yaqutils
    tools/mstand/mstand_utils
    tools/mstand/mstand_structs
    tools/mstand/user_plugins
    tools/mstand/sbs
)

PY_SRCS(
    NAMESPACE experiment_pool
    __init__.py
    criteria_result.py
    experiment.py
    experiment_for_calc.py
    filter_metric.py
    meta.py
    metric_result.py
    metric_result_for_calc.py
    observation.py
    observation_ut.py
    pool.py
    pool_exception.py
    pool_helpers.py
    deserializer/__init__.py
    deserializer/_v0.py
    deserializer/_v1.py
)

END()

RECURSE_FOR_TESTS(
    tests
)
