PY3_LIBRARY()

OWNER(g:antirobot)

PY_SRCS(
    __init__.py
)

PEERDIR(
    library/python/django
    contrib/python/psycopg2
    contrib/python/sqlalchemy/sqlalchemy-1.4
    contrib/python/ipaddr
    antirobot/cbb/cbb_django/cbb/models
    antirobot/cbb/cbb_django/cbb/library
)

END()
