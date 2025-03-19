PY3TEST()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    cloud/mdb/internal/python/compute/instances
    contrib/python/pytest
)

TEST_SRCS(
    test_api.py
    test_models.py
    test_platforms.py
)

END()
