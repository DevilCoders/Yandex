OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PY_SRCS(
    NAMESPACE dbaas_internal_api.modules
    __init__.py
)

PEERDIR(
    cloud/mdb/dbaas-internal-api-image/dbaas_internal_api/modules/clickhouse
    cloud/mdb/dbaas-internal-api-image/dbaas_internal_api/modules/hadoop
    cloud/mdb/dbaas-internal-api-image/dbaas_internal_api/modules/mongodb
    cloud/mdb/dbaas-internal-api-image/dbaas_internal_api/modules/mysql
    cloud/mdb/dbaas-internal-api-image/dbaas_internal_api/modules/postgres
    cloud/mdb/dbaas-internal-api-image/dbaas_internal_api/modules/redis
)

END()

RECURSE(
    clickhouse
    hadoop
    mongodb
    mysql
    postgres
    redis
)
