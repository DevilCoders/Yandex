OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PY_SRCS(
    NAMESPACE tests.providers
    __init__.py
)

PEERDIR(
    cloud/mdb/dbaas-internal-api-image/dbaas_internal_api
    contrib/python/dateutil
)

END()
