PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

ALL_PY_SRCS()

PEERDIR(
    contrib/python/click
    cloud/mdb/solomon-charts/internal/lib
)

END()
