OWNER(g:mdb)

PY23_LIBRARY()

STYLE_PYTHON()

PY_SRCS(
    NAMESPACE cloud.mdb.salt_tests.common
    __init__.py
    arc_utils.py
    assertion.py
    mocks.py
)

PEERDIR(
    contrib/python/lxml
    contrib/python/mock
)

IF (PYTHON2)
    PEERDIR(
        contrib/python/typing
    )
ENDIF()

NEED_CHECK()

END()

RECURSE_FOR_TESTS(
    tests
)
