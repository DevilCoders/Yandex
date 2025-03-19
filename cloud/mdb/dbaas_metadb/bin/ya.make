PY3_PROGRAM(populate_table)

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    contrib/python/psycopg2
)

PY_SRCS(
    MAIN
    populate_table.py
)

END()
