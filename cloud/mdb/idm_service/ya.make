PY3_PROGRAM(mdb_idm_service)

STYLE_PYTHON()

OWNER(g:mdb)

PY_SRCS(
    MAIN
    run.py
)

PEERDIR(
    cloud/mdb/idm_service/idm_service
)

END()

RECURSE(
    idm_service
    tests
    uwsgi
)
