PY3_PROGRAM(set_default)

OWNER(g:cloud-iam)

PEERDIR(
    contrib/python/colorama

    ydb/public/sdk/python
)

PY_MAIN(set_default)

PY_SRCS(
    TOP_LEVEL
    set_default.py
    queries.py
)

END()
