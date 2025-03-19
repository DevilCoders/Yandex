PY3TEST()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    cloud/mdb/dbaas_worker/test/mocks
    cloud/mdb/dbaas_worker/test/tasks
    cloud/mdb/dbaas_worker/test/tasks/sqlserver
    contrib/python/pytest-mock
)

FORK_SUBTESTS()

SPLIT_FACTOR(64)

TEST_SRCS(
    test_modify.py
)

END()
