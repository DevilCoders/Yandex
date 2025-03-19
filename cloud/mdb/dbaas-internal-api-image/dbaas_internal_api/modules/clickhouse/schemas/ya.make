OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PY_SRCS(
    NAMESPACE dbaas_internal_api.modules.clickhouse.schemas
    __init__.py
    backups.py
    clusters.py
    console.py
    databases.py
    format_schemas.py
    hosts.py
    models.py
    resource_presets.py
    shards.py
    users.py
)

END()
