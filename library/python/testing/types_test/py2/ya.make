PY23_LIBRARY()

OWNER(
    dim-gonch
    npytincev
    g:logos
)

PY_SRCS(__init__.py)

TEST_SRCS(types_test.py)

IF(PYTHON2)
    PEERDIR(
        contrib/python/typing
    )
ENDIF()


PEERDIR(
    library/python/testing/types_test/py2/config
    library/python/sfx
)

END()

RECURSE_FOR_TESTS(
   tests
)
