OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PY_SRCS(
    NAMESPACE commands
    __init__.py
    crash_prestart_when_persistence_off.py
    dump.py
    get_config_option.py
    is_alive.py
    remove_aof.py
    run_save_when_persistence_off.py
    stop_and_wait.py
    wait_started.py
    wait_replica_synced.py
)

PEERDIR(
    cloud/mdb/salt/salt/components/redis/common/conf/redisctl/utils
    cloud/mdb/salt/salt/components/redis/common/conf/redisctl/common
)

END()
