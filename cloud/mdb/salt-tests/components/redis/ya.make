PY3TEST()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    cloud/mdb/internal/python/pytest
    cloud/mdb/salt/salt/components/redis/common/conf/redisctl
    cloud/mdb/salt-tests/common
    contrib/python/mock
    contrib/python/redis
    contrib/python/PyHamcrest
    library/python/testing/yatest_common
)

DATA(arcadia/cloud/mdb/salt-tests/components/redis/test_data)

TEST_SRCS(
    test_redisctl_crash_prestart_when_persistence_off.py
    test_redisctl_is_alive.py
    test_redisctl_wait_started.py
    test_redisctl_wait_replica_synced.py
    test_redisctl_stop_and_wait.py
    test_redisctl_remove_aof.py
    test_redisctl_get_config_options.py
    test_redisctl_dump.py
    test_redisctl_run_save_when_persistence_off.py
    test_utils.py
)

TIMEOUT(60)

SIZE(SMALL)

END()
