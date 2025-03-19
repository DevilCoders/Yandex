PY3_PROGRAM(yc-iam-autodelete)

OWNER(g:cloud-iam)

PEERDIR(
    contrib/python/colorama

    ydb/public/sdk/python
)

PY_MAIN(autodelete)

PY_SRCS(
    TOP_LEVEL
    autodelete.py
)

END()
