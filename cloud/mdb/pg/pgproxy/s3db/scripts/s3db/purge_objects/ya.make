PY2_PROGRAM()

OWNER(g:mdb g:s3)

PEERDIR(
    cloud/mdb/pg/pgproxy/s3db/scripts/util
    contrib/python/python-daemon
    contrib/python/psycopg2
)

PY_MAIN(purge_objects)

PY_SRCS(
    TOP_LEVEL
    purge_objects.py
)

END()
