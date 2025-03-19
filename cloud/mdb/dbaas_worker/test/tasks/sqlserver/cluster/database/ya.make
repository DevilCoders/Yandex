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
    test_backup_export.py
    test_backup_import.py
    test_create.py
    test_delete.py
    test_modify.py
    test_restore.py
)

END()
