PY2TEST()

OWNER(g:cloud)

PEERDIR(
    cloud/gauthling/yc_requests/lib
)

TEST_SRCS(
    test_signing.py
)

END()
