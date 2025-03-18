PY23_LIBRARY()

OWNER(g:passport_infra)

PEERDIR(
    contrib/python/mock
    contrib/python/six
    library/cpp/tvmauth/client
)

IF(PYTHON2)
    PEERDIR(contrib/python/enum34)
ENDIF()
PEERDIR(contrib/python/urllib3)

PY_SRCS(
    TOP_LEVEL
    tvmauth/tvmauth_pymodule.pyx
    tvmauth/__init__.py
    tvmauth/deprecated.py
    tvmauth/exceptions.py
    tvmauth/mock.py
    tvmauth/unittest.py
    tvmauth/utils.py
)

END()

RECURSE_FOR_TESTS(
    examples
    so
    src/ut
)

IF (NOT MUSL AND NOT OS_WINDOWS AND NOT SANITIZER_TYPE)
    RECURSE_FOR_TESTS(
        src/ut_without_sanitizer
    )
ENDIF()
