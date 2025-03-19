PY23_TEST()
OWNER(g:mdb)

PEERDIR(
    cloud/mdb/salt/salt/_states
    cloud/mdb/salt/salt/_modules
    cloud/mdb/salt-tests/common
    contrib/python/mock
)

TEST_SRCS(
    test_rebalance.py
)

TIMEOUT(60)
SIZE(SMALL)
END()
