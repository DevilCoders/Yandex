PY23_TEST()

OWNER(g:reactor)

TEST_SRCS(
    __init__.py
    test_schemas.py
    test_api.py
    test_builders.py
)

PEERDIR(
    library/python/reactor/client
    library/python/reactor/client/tests/helpers
    contrib/python/requests-mock
    contrib/python/pytz
    contrib/python/six
)

DATA(arcadia/library/python/reactor/client/tests/resourses)

END()
