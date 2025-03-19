PY2TEST()

OWNER(g:cloud)

PEERDIR(
    cloud/gauthling/gauthling_daemon_tornado/lib
    cloud/gauthling/gauthling_daemon_mock/lib
    contrib/python/tornado/tornado-4
    contrib/python/pytest-tornado
)

TEST_SRCS(
    conftest.py
    test_client.py
)

NO_CHECK_IMPORTS()

END()
