PY2TEST()

OWNER(g:cloud)

PEERDIR(
    cloud/gauthling/yc_auth/lib
    cloud/gauthling/yc_auth_tornado/lib
    contrib/python/mock
    contrib/python/tornado/tornado-4
    contrib/python/pytest-tornado
    contrib/python/six
)

TEST_SRCS(
    conftest.py
    test_authentication.py
    test_authorization.py
)

NO_CHECK_IMPORTS()


END()
