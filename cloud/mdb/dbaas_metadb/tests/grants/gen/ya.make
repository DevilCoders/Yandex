PY3_PROGRAM(gen_grants_users)

STYLE_PYTHON()

OWNER(g:mdb)

PY_SRCS(
    MAIN
    gen_grants_users.py
)

PEERDIR(
    cloud/mdb/dbaas_metadb/tests/grants/lib
)

END()
