OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PY_SRCS(
    NAMESPACE dbaas_internal_api.modules.redis
    __init__.py
    backups.py
    billing.py
    charts.py
    console.py
    constants.py
    create.py
    delete.py
    failover.py
    hosts.py
    info.py
    maintenance.py
    modify.py
    move.py
    operations.py
    pillar.py
    rebalance.py
    restore.py
    shards.py
    start.py
    stop.py
    traits.py
    types.py
    utils.py
)

PEERDIR(
    cloud/mdb/dbaas-internal-api-image/dbaas_internal_api/modules/redis/schemas
)

END()

RECURSE(
    schemas
)
