OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PY_SRCS(
    NAMESPACE dbaas_internal_api.modules.mongodb
    __init__.py
    backups.py
    billing.py
    charts.py
    console.py
    constants.py
    create.py
    databases.py
    delete.py
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
    shards.py
    start.py
    stop.py
    traits.py
    types.py
    users.py
    utils.py
)

END()
