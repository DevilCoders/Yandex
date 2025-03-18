PY23_TEST()

OWNER(g:passport_infra)

PEERDIR(
    library/python/tvmauth
)

TEST_SRCS(
    test_client.py
    test_common.py
    test_service.py
    test_user.py
)

DATA(arcadia/library/cpp/tvmauth/client/ut/files)

END()
