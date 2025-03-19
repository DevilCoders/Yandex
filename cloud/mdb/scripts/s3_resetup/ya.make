PY3_PROGRAM(s3-resetup)

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    contrib/python/requests
    contrib/python/retrying
)

PY_MAIN(s3_resetup:main)

PY_SRCS(
    TOP_LEVEL
    s3_resetup.py
)

END()
