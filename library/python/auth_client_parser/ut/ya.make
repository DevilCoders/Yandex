PY23_TEST()

OWNER(g:passport_infra)

PEERDIR(
    library/python/auth_client_parser
)

TEST_SRCS(test.py)

END()
