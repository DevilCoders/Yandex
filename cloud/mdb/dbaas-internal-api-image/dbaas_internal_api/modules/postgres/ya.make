OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PY_SRCS(
    NAMESPACE dbaas_internal_api.modules.postgres
    __init__.py
    alerts.py
    backups.py
    billing.py
    charts.py
    console.py
    constants.py
    create.py
    databases.py
    delete.py
    failover.py
    fields.py
    host_pillar.py
    hosts.py
    info.py
    maintenance.py
    modify.py
    move.py
    operations.py
    pillar.py
    restore.py
    schema_defaults.py
    schemas.py
    search_render.py
    start.py
    stop.py
    traits.py
    types.py
    users.py
    utils.py
    validation.py
)

PEERDIR(
    cloud/mdb/dbaas-internal-api-image/dbaas_internal_api/modules/postgres/api
    cloud/bitbucket/public-api/yandex/cloud/events/mdb/postgresql
)

END()

RECURSE(
    api
)
