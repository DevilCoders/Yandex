OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PY_SRCS(
    NAMESPACE dbaas_internal_api.modules.clickhouse
    __init__.py
    backups.py
    billing.py
    charts.py
    console.py
    constants.py
    create.py
    databases.py
    delete.py
    dictionaries.py
    format_schemas.py
    hosts.py
    info.py
    maintenance.py
    models.py
    modify.py
    move.py
    operations.py
    pillar.py
    restore.py
    schema_defaults.py
    search_renders.py
    shards.py
    start.py
    stop.py
    traits.py
    users.py
    utils.py
    zookeeper.py
)

PEERDIR(
    cloud/mdb/dbaas-internal-api-image/dbaas_internal_api/modules/clickhouse/schemas
)

END()

RECURSE(
    schemas
)
