PY2_PROGRAM()

OWNER(g:mdb g:s3)

PEERDIR(
    cloud/mdb/pg/pgproxy/s3db/scripts/util
    contrib/python/psycopg2
)

PY_MAIN(update_bucket_stat)

PY_SRCS(
    TOP_LEVEL
    update_bucket_stat.py
)

END()
