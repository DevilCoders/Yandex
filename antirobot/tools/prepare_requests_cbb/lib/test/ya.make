PY3TEST()

SIZE(SMALL)

OWNER(
    g:antirobot
)

TEST_SRCS(
    test_service_identifier.py
)

PEERDIR(
    antirobot/tools/prepare_requests_cbb/lib
)

END()
