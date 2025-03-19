PY23_TEST()
OWNER(g:mdb)

PEERDIR(
    cloud/mdb/internal/python/pytest
    cloud/mdb/salt/salt/_modules
    cloud/mdb/salt-tests/common
    contrib/python/mock
    contrib/python/python-magic
)

TEST_SRCS(
    test_auth.py
    test_calculated_attributes.py
    test_extension.py
    test_license.py
    test_renderers.py
    test_misc.py
)

TIMEOUT(60)
SIZE(SMALL)
END()
