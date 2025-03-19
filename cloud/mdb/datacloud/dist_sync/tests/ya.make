OWNER(g:mdb)

PY3TEST()

STYLE_PYTHON()

PEERDIR(
    cloud/mdb/datacloud/dist_sync
)

TEST_SRCS(
    test_dist_sync.py
)

END()
