OWNER(g:mdb)

PY23_TEST()


TEST_SRCS(
    test_mdb_salt_returner.py
)

PEERDIR(
    contrib/python/mock
    contrib/python/pytest
    contrib/python/PyYAML
    contrib/python/requests-mock

    cloud/mdb/mdb-config-salt/src
)

END()
