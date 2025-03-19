OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PY_SRCS(
    NAMESPACE dbaas_internal_api.dbs
    __init__.py
    pool.py
    query.py
)

PEERDIR(
    cloud/mdb/dbaas-internal-api-image/dbaas_internal_api/dbs/postgresql
)

END()

RECURSE(
    postgresql
)
