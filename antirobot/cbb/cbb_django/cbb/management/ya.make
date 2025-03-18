PY3_LIBRARY()

OWNER(g:antirobot)

PY_SRCS(
    __init__.py
)

PEERDIR(
    library/python/django
    antirobot/cbb/cbb_django/cbb/library
    antirobot/cbb/cbb_django/cbb/management/commands
    contrib/python/sqlalchemy/sqlalchemy-1.4
    contrib/python/ipaddr
)

END()
