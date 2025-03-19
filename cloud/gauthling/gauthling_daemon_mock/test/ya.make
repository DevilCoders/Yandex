PY2TEST()

OWNER(g:cloud)

PEERDIR(
    cloud/gauthling/gauthling_daemon/lib
    cloud/gauthling/gauthling_daemon_mock/lib
    contrib/python/requests
)

TEST_SRCS(
    conftest.py
    test_control_server.py
)

END()
