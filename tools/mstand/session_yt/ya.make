OWNER(
    g:mstand
)

PY3_LIBRARY()

PEERDIR(
    library/python/resource
    statbox/nile
    yql/library/python

    quality/yaqlib/yaqlibenums
    quality/yaqlib/yaqutils

    tools/mstand/mstand_utils
    tools/mstand/mstand_structs
    tools/mstand/user_plugins
    tools/mstand/session_metric
    tools/mstand/squeeze_lib/pylib
)

PY_SRCS(
    NAMESPACE session_yt
    __init__.py
    squeeze_yt.py
    versions_yt.py
    metric_yt.py
)

RESOURCE_FILES(
    tools/mstand/yql_scripts/zen/squeeze_lib.sql
    tools/mstand/yql_scripts/zen/run_squeeze.sql
)

END()

RECURSE_FOR_TESTS(
    tests
)
