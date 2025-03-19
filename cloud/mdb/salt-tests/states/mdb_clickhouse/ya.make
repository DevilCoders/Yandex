PY23_TEST()
OWNER(g:mdb)

NO_DOCTESTS()

PEERDIR(
    cloud/mdb/internal/python/pytest
    cloud/mdb/salt/salt/_states
    cloud/mdb/salt/salt/_modules
    cloud/mdb/salt-tests/common
    contrib/python/mock
)

TEST_SRCS(
    test_sync_databases.py
    test_reload_config.py
    test_reload_dictionaries.py
    test_resetup_required.py
    test_ensure_mdb_tables.py
    test_sql_access_management.py
    test_cleanup_log_tables.py
)

TIMEOUT(60)
SIZE(SMALL)
END()
