PY3_LIBRARY()

OWNER(g:mdb)

PEERDIR(
    contrib/python/django/django-3
)

PY_SRCS(
    0001_initial.py
    __init__.py
)

NO_CHECK_IMPORTS()

END()
