PY2_PROGRAM()

OWNER(g:mdb g:s3)

PEERDIR(
    cloud/mdb/pg/pgproxy/s3db/scripts/util
    contrib/python/psycopg2
    library/python/testing/yatest_common
)

PY_MAIN(copy_delete_queue)

PY_SRCS(
    TOP_LEVEL
    copy_delete_queue.py
)

END()
