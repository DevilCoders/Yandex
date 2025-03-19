PY3TEST()
OWNER(g:cloud-infra)

TEST_SRCS(
    test_req.py
    test_ycinfra.py
)

PEERDIR(
    cloud/infra/packages/lib
    contrib/python/parameterized
)

END()
