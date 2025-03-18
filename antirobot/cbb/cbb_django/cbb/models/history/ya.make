PY3_LIBRARY()

OWNER(g:antirobot)

PY_SRCS(
    __init__.py
    history_ipv4.py
    history_ipv6.py
    history_re.py
    history_txt.py
    limbo_ipv4.py
    limbo_ipv6.py
    limbo_re.py
    limbo_txt.py
    prototype_ip.py
    prototype_line.py
    prototype.py
)

PEERDIR(
    library/python/django
    antirobot/cbb/cbb_django/cbb/library
    contrib/python/sqlalchemy/sqlalchemy-1.4
    contrib/python/ipaddr
)

END()
