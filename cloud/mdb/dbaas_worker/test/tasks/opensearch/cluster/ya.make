PY3TEST()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    cloud/mdb/dbaas_worker/test/mocks
    cloud/mdb/dbaas_worker/test/tasks
    cloud/mdb/dbaas_worker/test/tasks/opensearch
    contrib/python/pytest-mock
)

FORK_SUBTESTS()

SPLIT_FACTOR(64)

TEST_SRCS(
    test_create.py
    test_create_backup.py
    test_delete.py
    test_delete_metadata.py
    test_maintenance.py
    test_metadata.py
    test_modify.py
    test_purge.py
    test_restore.py
    test_start.py
    test_stop.py
    test_upgrade.py
)

END()
