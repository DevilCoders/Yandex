PY2_PROGRAM()

OWNER(g:mdb g:s3)

PEERDIR(
    cloud/mdb/pg/pgproxy/s3db/scripts/util
    contrib/python/psycopg2
)

PY_MAIN(chunk_creator)

PY_SRCS(
    TOP_LEVEL
    chunk_creator.py
)

END()
