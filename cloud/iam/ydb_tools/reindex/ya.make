PY3_PROGRAM(reindex)

OWNER(g:cloud-iam)

PEERDIR(
    contrib/python/colorama

    ydb/public/sdk/python
)

PY_MAIN(reindex)

PY_SRCS(
    TOP_LEVEL
    reindex.py
)

END()
