PY3TEST()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    cloud/mdb/internal/python/compute/clouds
    contrib/python/pytest-mock
)

TEST_SRCS(test_api.py)

END()
