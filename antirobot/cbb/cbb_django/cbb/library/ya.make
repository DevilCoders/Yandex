PY3_LIBRARY()

OWNER(g:antirobot)

PY_SRCS(
    antirobot_groups.py
    common.py
    db.py
    errors.py
    fields.py
    data.py
    hooks.py
    lock.py
    notify.py
    antiddos.pyx
)

PEERDIR(
    antirobot/tools/antiddos/lib
    library/python/django
    contrib/python/psycopg2
    contrib/python/sqlalchemy/sqlalchemy-1.4
    contrib/python/ipaddr
    contrib/python/ipaddr
    library/python/ylock
    library/python/django-idm-api
    library/python/resource
)

RESOURCE(
    antirobot/config/service_config.json service_config.json
)

END()
