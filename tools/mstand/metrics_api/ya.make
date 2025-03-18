PY3_LIBRARY()

OWNER(
    g:mstand
)

PY_SRCS(
    NAMESPACE metrics_api
    __init__.py
    common.py
    offline.py
    online.py
    helpers.py
)

PEERDIR(
    quality/yaqlib/yaqutils
    tools/mstand/experiment_pool
    tools/mstand/mstand_utils
    tools/mstand/serp
    tools/mstand/user_plugins
)

END()

RECURSE_FOR_TESTS(
    tests
)
