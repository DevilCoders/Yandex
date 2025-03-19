OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PEERDIR(
    library/python/testing/recipe
    library/python/testing/yatest_common
    contrib/python/yandex-pgmigrate
)

PY_SRCS(
    __init__.py
    persistence.py
)

END()
