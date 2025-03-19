PY2_PROGRAM()

OWNER(g:mdb g:s3)

PEERDIR(
    cloud/mdb/pg/pgproxy/s3db/scripts/util
    contrib/python/psycopg2
)

PY_MAIN(s3_closer)

PY_SRCS(
    TOP_LEVEL
    s3_closer.py
)

END()
