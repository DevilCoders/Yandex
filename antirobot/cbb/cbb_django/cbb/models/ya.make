PY3_LIBRARY()

OWNER(g:antirobot)

PY_SRCS(
    __init__.py
    group.py
    user_role.py
)

PEERDIR(
    antirobot/cbb/cbb_django/cbb/models/block
    antirobot/cbb/cbb_django/cbb/models/history
    antirobot/cbb/cbb_django/cbb/library
    contrib/python/sqlalchemy/sqlalchemy-1.4
    contrib/python/ipaddr
    contrib/python/dateutil
)

END()
