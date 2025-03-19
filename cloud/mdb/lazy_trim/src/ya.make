PY3_PROGRAM(lazy_trim)

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    contrib/python/lockfile
)

PY_MAIN(lazy_trim:main)

PY_SRCS(
    TOP_LEVEL
    lazy_trim.py
)

END()
