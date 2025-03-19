OWNER(g:mdb)

PY23_TEST()

PEERDIR(
    cloud/mdb/salt/salt/_modules
)

TEST_SRCS(
    test_certificate_manager.py
)

END()
