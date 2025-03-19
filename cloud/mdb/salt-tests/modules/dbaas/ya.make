PY23_TEST()
OWNER(g:mdb)

PEERDIR(
    cloud/mdb/internal/python/pytest
    cloud/mdb/salt/salt/_modules
    contrib/python/mock
)

TEST_SRCS(
    test_data_disk.py
    test_environ_fun.py
    test_misc_fun.py
)

TIMEOUT(60)
SIZE(SMALL)
END()
