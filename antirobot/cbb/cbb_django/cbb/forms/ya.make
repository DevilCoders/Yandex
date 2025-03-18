PY3_LIBRARY()

OWNER(g:antirobot)

PY_SRCS(
    __init__.py
    admin.py
    api.py
)

PEERDIR(
    library/python/django
    contrib/python/psycopg2
    contrib/python/sqlalchemy/sqlalchemy-1.4
    contrib/python/ipaddr

    antirobot/cbb/cbb_django/cbb/library
    antirobot/cbb/cbb_django/cbb/models
)

END()
