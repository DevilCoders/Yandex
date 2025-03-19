OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PY_SRCS(
    NAMESPACE dbaas_internal_api.modules.redis.schemas
    __init__.py
    backups.py
    clusters.py
    configs.py
    console.py
    hosts.py
    resource_presets.py
    resources.py
    shards.py
)

END()
