PY3_LIBRARY()

OWNER(
    g:mstand
)

PY_SRCS(
    NAMESPACE sample_metrics
    __init__.py
)

PEERDIR(
    tools/mstand/metrics_api
    tools/mstand/sample_metrics/offline
    tools/mstand/sample_metrics/online
)

END()

RECURSE_FOR_TESTS(
    online/tests
)
