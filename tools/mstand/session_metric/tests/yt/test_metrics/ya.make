PY3_LIBRARY()

OWNER(g:mstand)

PEERDIR(
    tools/mstand/experiment_pool
)

PY_SRCS(
    NAMESPACE test_metrics
    __init__.py
    default_metric.py
)

END()
