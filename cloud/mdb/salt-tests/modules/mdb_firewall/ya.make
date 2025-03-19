PY23_TEST()
OWNER(g:mdb)

PEERDIR(
    cloud/mdb/internal/python/pytest
    cloud/mdb/salt/salt/_modules
    cloud/mdb/salt-tests/common
    contrib/python/mock
)

TEST_SRCS(
    test_render_external_access_config.py
)

TIMEOUT(60)
SIZE(SMALL)
END()
