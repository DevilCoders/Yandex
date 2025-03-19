OWNER(g:mdb)

PY3_PROGRAM(dbaas-e2e)

STYLE_PYTHON()

PY_SRCS(
    MAIN
    run.py
)

PEERDIR(
    cloud/mdb/e2e-tests/dbaas_e2e
)

END()

RECURSE(
    dbaas_e2e
)
