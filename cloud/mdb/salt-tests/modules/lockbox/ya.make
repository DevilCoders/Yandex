OWNER(g:mdb)

PY3TEST()

STYLE_PYTHON()

PEERDIR(
    cloud/mdb/internal/python/pytest
    cloud/mdb/salt/salt/_modules
    cloud/mdb/salt-tests/common
)

TEST_SRCS(test_lockbox.py)

END()
