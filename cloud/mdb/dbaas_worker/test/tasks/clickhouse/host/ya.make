PY3TEST()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    cloud/mdb/dbaas_worker/test/mocks
    cloud/mdb/dbaas_worker/test/tasks
    cloud/mdb/dbaas_worker/test/tasks/clickhouse
    contrib/python/pytest-mock
)

FORK_SUBTESTS()

SPLIT_FACTOR(4)

TEST_SRCS(
    test_create.py
    test_modify.py
    test_delete.py
    test_zookeeper_create.py
    test_zookeeper_delete.py
)

END()
