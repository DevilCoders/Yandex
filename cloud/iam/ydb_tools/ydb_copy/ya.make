PY3_PROGRAM(ydb_copy)

OWNER(g:cloud-iam)

PEERDIR(
    contrib/python/colorama

    ydb/public/sdk/python
)

PY_MAIN(ydb_copy)

PY_SRCS(
    TOP_LEVEL
    ydb_copy.py
)

END()
