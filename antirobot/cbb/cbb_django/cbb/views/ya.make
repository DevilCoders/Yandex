PY3_LIBRARY()

OWNER(g:antirobot)

PY_SRCS(
    __init__.py
    admin.py
    api.py
    antiddos.py
)

PEERDIR(
    antirobot/cbb/cbb_django/cbb/forms
    antirobot/cbb/cbb_django/cbb/library
    antirobot/cbb/cbb_django/cbb/models
    library/python/deprecated/ticket_parser2
    library/python/django
    library/python/ylock
    yt/python/client
)

END()
