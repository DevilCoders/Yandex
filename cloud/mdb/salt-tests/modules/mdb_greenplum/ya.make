PY3TEST()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    cloud/mdb/internal/python/pytest
    cloud/mdb/salt/salt/_modules
    cloud/mdb/salt-tests/common
)

TEST_SRCS(test_mdb_greenplum.py)

TIMEOUT(60)

SIZE(SMALL)

END()
