OWNER(g:mdb)

PY23_TEST()

PEERDIR(
    cloud/mdb/salt/salt/_modules
    contrib/python/requests-mock
)

TEST_SRCS(
    test_compute_metadata.py
)

END()
