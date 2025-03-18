PY23_LIBRARY()

OWNER(g:passport_infra)

PEERDIR(
    library/cpp/auth_client_parser
)

IF(PYTHON2)
    PEERDIR(contrib/python/enum34)
ENDIF()

PY_SRCS(
    TOP_LEVEL
    auth_client_parser.pyx
)

END()

RECURSE_FOR_TESTS(
    examples
    ut
)
