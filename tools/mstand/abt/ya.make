OWNER(g:mstand)

PY3_LIBRARY()

PY_SRCS(
    NAMESPACE abt
    __init__.py
    result_getter.py
    result_parser.py
)

PEERDIR(
    quality/ab_testing/scripts/ems/library/ab_calculations
    quality/ab_testing/scripts/nirvana/get_calculations/lib

    quality/yaqlib/yaqutils

    tools/mstand/experiment_pool
    tools/mstand/user_plugins
)

END()

RECURSE_FOR_TESTS(
    tests
)
