PY23_TEST()
OWNER(g:mdb)

PEERDIR(
    cloud/mdb/salt/salt/_modules
    contrib/python/mock
)

TEST_SRCS(
    test_generate_config.py
    test_compare_config.py
    test_load_config.py
)

TIMEOUT(60)
SIZE(SMALL)
END()
