PY2_PROGRAM()

OWNER(g:mdb g:s3)

PEERDIR(
    cloud/mdb/pg/pgproxy/s3db/scripts/util
    contrib/python/psycopg2
)

PY_MAIN(finish_prepared_xacts)

PY_SRCS(
    TOP_LEVEL
    finish_prepared_xacts.py
)

END()
