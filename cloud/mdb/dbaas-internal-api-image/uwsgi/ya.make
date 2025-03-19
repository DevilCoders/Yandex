PY3_PROGRAM(internal-api.wsgi)

STYLE_PYTHON()

OWNER(g:mdb)

PY_MAIN(pyuwsgi:run)

PY_SRCS(
    NAMESPACE cloud.mdb.dbaas_internal_api.uwsgi
    application.py
)

PEERDIR(
    contrib/python/uwsgi
    cloud/mdb/dbaas-internal-api-image/dbaas_internal_api
    cloud/mdb/dbaas_python_common
)

# We instantiate int-api app and it fails on logging initialization (FileNotFoundError .../api.log)

NO_CHECK_IMPORTS(
    cloud.mdb.dbaas_internal_api.uwsgi.*
)

END()
