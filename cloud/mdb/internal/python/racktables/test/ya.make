PY3TEST()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    cloud/mdb/internal/python/racktables
)

TEST_SRCS(test_client.py)

END()
