PY3_LIBRARY()

OWNER(g:antirobot)

PY_SRCS(
    __init__.py
    ipv4.py
    ipv6.py
    prototype_ip.py
    prototype_line.py
    prototype.py
    re.py
    txt.py
)

PEERDIR(
    library/python/django
    antirobot/cbb/cbb_django/cbb/library
    contrib/python/sqlalchemy/sqlalchemy-1.4
    contrib/python/ipaddr
)

END()
