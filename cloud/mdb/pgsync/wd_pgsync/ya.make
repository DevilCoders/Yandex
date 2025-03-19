OWNER(g:mdb)

PY3_PROGRAM(wd_pgsync)

STYLE_PYTHON()

PY_MAIN(cloud.mdb.pgsync.wd_pgsync.wd_pgsync:main)

PY_SRCS(wd_pgsync.py)

PEERDIR(
    contrib/python/kazoo
    contrib/python/psycopg2
    contrib/python/lockfile
    contrib/python/python-daemon
    cloud/mdb/pgsync
)

END()
