PY23_TEST()
OWNER(g:mdb)

PEERDIR(
    cloud/mdb/salt/salt/_modules
)

TEST_SRCS(
    test_mysql_hashes.py
)

TIMEOUT(60)
SIZE(SMALL)
END()
