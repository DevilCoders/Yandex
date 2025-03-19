OWNER(g:mdb)

PY3TEST()

STYLE_PYTHON()

PEERDIR(
    cloud/mdb/salt/salt/components/datacloud/dist-sync
)

TEST_SRCS(
    test_dist_sync.py
)

END()
