PY23_TEST()
OWNER(g:mdb)
NO_DOCTESTS()

PEERDIR(
    cloud/mdb/internal/python/pytest
    cloud/mdb/salt/salt/_states
    cloud/mdb/salt/salt/_modules
    cloud/mdb/salt-tests/common
    contrib/python/mock
)

TEST_SRCS(
    test_ensure_plugins.py
    test_ensure_keystore.py
    test_ensure_license.py
)

TIMEOUT(60)
SIZE(SMALL)
END()
