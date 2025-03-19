PY23_TEST()
OWNER(g:mdb)

PEERDIR(
    cloud/mdb/internal/python/pytest
    cloud/mdb/salt/salt/_states
    cloud/mdb/salt-tests/common
    contrib/python/mock
)

TEST_SRCS(
    test_object_present.py
)

TIMEOUT(60)
SIZE(SMALL)
END()
