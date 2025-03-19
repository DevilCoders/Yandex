OWNER(g:mdb)

PY23_TEST()

PEERDIR(
    cloud/mdb/internal/python/pytest
    cloud/mdb/salt-tests/common
)

TEST_SRCS(
    test_mock_version_cmp.py
)

TIMEOUT(60)
SIZE(SMALL)
END()
