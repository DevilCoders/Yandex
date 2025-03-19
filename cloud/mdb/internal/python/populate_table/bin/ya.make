PY3_PROGRAM(populate_table)

STYLE_PYTHON()

OWNER(g:mdb)

PY_MAIN(cloud.mdb.internal.python.populate_table.bin.populate_table:main)

PY_SRCS(populate_table.py)

PEERDIR(
    cloud/mdb/internal/python/populate_table/internal
)

END()
