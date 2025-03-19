PY2TEST()

OWNER(g:cloud)

PEERDIR(
    cloud/gauthling/gauthling_daemon/lib
    cloud/gauthling/gauthling_daemon_mock/lib
)

TEST_SRCS(
    conftest.py
    test_client.py
)

END()
