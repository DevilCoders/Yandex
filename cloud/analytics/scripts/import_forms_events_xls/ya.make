PY3_PROGRAM()

OWNER(g:cloud_analytics)

PEERDIR(
    contrib/python/pandas
    contrib/python/click
    contrib/python/xlrd
    yt/python/client_with_rpc
)

PY_SRCS(
    MAIN __init__.py
    schema.py
)

END()
