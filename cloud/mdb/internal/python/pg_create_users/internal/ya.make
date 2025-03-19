PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    contrib/python/boto3
    contrib/python/requests
    contrib/python/psycopg2
)

PY_SRCS(
    __init__.py
    base.py
)

END()
