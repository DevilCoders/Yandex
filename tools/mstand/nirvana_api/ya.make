OWNER(
    g:mstand
)

PY3_LIBRARY()

PEERDIR(
    quality/yaqlib/yaqutils
    tools/mstand/mstand_enums
)

PY_SRCS(
    NAMESPACE nirvana_api
    __init__.py
    workflow_runner.py
)

END()

RECURSE_FOR_TESTS(
    tests
)
