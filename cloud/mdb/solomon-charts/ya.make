PY3_PROGRAM(solomon)

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    contrib/python/click
    cloud/mdb/solomon-charts/internal/lib
    cloud/mdb/solomon-charts/internal/cli
)

PY_MAIN(cloud.mdb.solomon-charts.solomon:cli)

PY_SRCS(solomon.py)

END()

RECURSE(
    internal
)
