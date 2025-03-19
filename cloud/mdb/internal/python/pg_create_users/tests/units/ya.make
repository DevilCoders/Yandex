OWNER(g:mdb)

PY3TEST()

STYLE_PYTHON()

PEERDIR(
    cloud/mdb/internal/python/pg_create_users/internal
)

TEST_SRCS(test_base.py)

END()
