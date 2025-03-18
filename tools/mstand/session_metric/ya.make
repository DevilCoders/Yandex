OWNER(
    g:mstand
)

PY3_LIBRARY()

PY_SRCS(
    NAMESPACE session_metric
    __init__.py
    lamps_helpers.py
    metric_calculator.py
    metric_runner.py
    metric_v2.py
    online_metric_main.py
    user_filters.py
    metric_runner_test.py
    metric_quick_check.py
)

PEERDIR(
    quality/yaqlib/yaqutils

    tools/mstand/adminka
    tools/mstand/mstand_utils
    tools/mstand/mstand_structs
    tools/mstand/mstand_metric_helpers
    tools/mstand/metrics_api
    tools/mstand/mstand_enums
    tools/mstand/user_plugins
    tools/mstand/session_squeezer

    # this depend is for arcadia build only
    tools/mstand/sample_metrics
)

END()

RECURSE_FOR_TESTS(
    tests
)
