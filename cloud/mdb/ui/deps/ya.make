PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    contrib/python/django/django-2.2
)

ALL_PY_SRCS(
    RECURSIVE
    TOP_LEVEL
)

INCLUDE(ya.resources.static)

END()
