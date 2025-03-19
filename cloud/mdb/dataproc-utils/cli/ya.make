OWNER(g:mdb)

PY3_PROGRAM(dataproc-utils)

STYLE_PYTHON()

PY_SRCS(
    MAIN
    main.py
)

PEERDIR(
    contrib/python/click
    contrib/python/psycopg2
    contrib/python/pyaml
    contrib/python/sshtunnel
)

END()
