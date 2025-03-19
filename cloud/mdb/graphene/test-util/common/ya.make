PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    contrib/python/psycopg2
)

PY_SRCS(
    NAMESPACE common
    pg_cluster.py
)

END()
