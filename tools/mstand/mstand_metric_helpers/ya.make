PY3_LIBRARY()

OWNER(
    g:mstand
)

PY_SRCS(
    NAMESPACE mstand_metric_helpers
    __init__.py
    common_metric_helpers.py
    online_metric_helpers.py
)

PEERDIR(
    quality/yaqlib/yaqutils
    tools/mstand/metrics_api
)

END()
