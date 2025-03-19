OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PEERDIR(
    cloud/bitbucket/public-api/yandex/cloud/events/mdb/mysql
)

PY_SRCS(
    NAMESPACE dbaas_internal_api.modules.mysql
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

END()
