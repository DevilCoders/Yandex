PY3_PROGRAM(pg-upgrade-non-dbaas)

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    contrib/python/psycopg2
    contrib/python/kazoo
    contrib/python/paramiko
    contrib/python/retrying
)

PY_MAIN(upgrade_pg:main)

PY_SRCS(
    TOP_LEVEL
    upgrade_pg.py
)

END()
