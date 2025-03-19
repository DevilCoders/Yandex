PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

ALL_PY_SRCS()

PEERDIR(
    cloud/mdb/internal/python/query_conf
)

INCLUDE(queries.inc)

END()
