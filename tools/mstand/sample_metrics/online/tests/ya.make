PY3TEST()

OWNER(
    g:mstand
)

TEST_SRCS(
    tools/mstand/sample_metrics/online/conftest.py
    tools/mstand/sample_metrics/online/metrics_ut.py
    tools/mstand/sample_metrics/online/metrics_and_helpers_ut.py
    tools/mstand/sample_metrics/online/test_metrics_for_services.py
    tools/mstand/sample_metrics/online/test_metrics_for_versions.py
)

DATA(
    arcadia/tools/mstand/sample_metrics/online/tests/data
)

PEERDIR(
    quality/yaqlib/yaqutils
    tools/mstand/experiment_pool
    tools/mstand/mstand_enums
    tools/mstand/mstand_metric_helpers
    tools/mstand/session_local
    tools/mstand/session_metric
)

SIZE(SMALL)

END()
