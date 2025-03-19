PY2_PROGRAM()

OWNER(g:mdb g:s3)

PEERDIR(
    cloud/mdb/pg/pgproxy/s3db/scripts/util
    contrib/python/psycopg2
)

PY_MAIN(merge_chunks)

PY_SRCS(
    TOP_LEVEL
    merge_chunks.py
)

END()
