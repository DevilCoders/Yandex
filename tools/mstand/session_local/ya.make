OWNER(
    g:mstand
)

PY3_LIBRARY()

PY_SRCS(
    NAMESPACE session_local
    __init__.py
    metric_local.py
    mr_attributes_local.py
    mr_merge_local.py
    squeeze_local.py
    versions_local.py
)

PEERDIR(
    quality/yaqlib/yaqutils

    tools/mstand/experiment_pool
    tools/mstand/mstand_structs
    tools/mstand/session_metric
    tools/mstand/session_squeezer
    tools/mstand/session_yt
)

END()

RECURSE_FOR_TESTS(
    tests
)
