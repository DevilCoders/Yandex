OWNER(g:mdb)

PY3_PROGRAM(gpsync)

STYLE_PYTHON()

PEERDIR(
    contrib/python/kazoo
    contrib/python/psycopg2
    contrib/python/lockfile
    contrib/python/python-daemon
)

PY_SRCS(
    __init__.py
    cli.py
    command_manager.py
    exceptions.py
    failover_election.py
    helpers.py
    main.py
    pg.py
    plugin.py
    plugins/pgbouncer.py
    plugins/upload_wals.py
    replication_manager.py
    sdnotify.py
    utils.py
    zk.py
)

PY_MAIN(cloud.mdb.gpsync:main)

END()

RECURSE(
    gpsync-util
)

RECURSE_FOR_TESTS(
    tests
)
