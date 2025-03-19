PY3_PROGRAM(dev-dbm)

STYLE_PYTHON()

OWNER(g:mdb)

PY_SRCS(__main__.py)

PEERDIR(
    cloud/mdb/dbm/internal
)

END()
