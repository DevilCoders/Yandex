PY3_PROGRAM(mdb-idm-service.wsgi)

STYLE_PYTHON()

OWNER(g:mdb)

PY_MAIN(pyuwsgi:run)

PY_SRCS(
    NAMESPACE cloud.mdb.idm_service.uwsgi
    application.py
)

PEERDIR(
    contrib/python/uwsgi
    cloud/mdb/idm_service/idm_service
)

END()
