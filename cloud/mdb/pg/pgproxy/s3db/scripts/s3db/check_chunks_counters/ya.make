PY2_PROGRAM()

OWNER(g:mdb g:s3)

PEERDIR(
    cloud/mdb/pg/pgproxy/s3db/scripts/util
    contrib/python/psycopg2
)

PY_MAIN(check_chunks_counters)

PY_SRCS(
    TOP_LEVEL
    check_chunks_counters.py
)

END()
