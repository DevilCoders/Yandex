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

SPLIT_FACTOR(15)

TEST_SRCS(
    test_add_zookeeper.py
    test_create.py
    test_create_backup.py
    test_delete.py
    test_delete_metadata.py
    test_maintenance.py
    test_modify.py
    test_move.py
    test_purge.py
    test_remove_zookeeper.py
    test_restore.py
    test_start.py
    test_stop.py
    test_update_tls_certs.py
    test_upgrade.py
)

END()
