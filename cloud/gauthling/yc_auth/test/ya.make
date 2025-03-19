PY2TEST()

OWNER(g:cloud)

PEERDIR(
    cloud/gauthling/yc_auth/lib
    contrib/python/mock
    contrib/python/Werkzeug
)

TEST_SRCS(
    conftest.py
    test_authentication.py
    test_authorization.py
    test_utils.py
)



END()
