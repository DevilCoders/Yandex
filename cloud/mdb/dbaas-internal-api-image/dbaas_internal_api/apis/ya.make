OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PY_SRCS(
    NAMESPACE dbaas_internal_api.apis
    __init__.py
    alerts.py
    backups.py
    config_auth.py
    create.py
    delete.py
    dynamic_handlers.py
    filters.py
    info.py
    maintenance.py
    marshal.py
    modify.py
    move.py
    operations.py
    operations_responser.py
    parse_kwargs.py
    quota.py
    resource_presets.py
    restore.py
    slb.py
    start.py
    stat.py
    stop.py
    support.py
    types.py
)

PEERDIR(
    cloud/mdb/dbaas-internal-api-image/dbaas_internal_api/apis/config
    cloud/mdb/dbaas-internal-api-image/dbaas_internal_api/apis/console
    cloud/mdb/dbaas-internal-api-image/dbaas_internal_api/apis/schemas
    cloud/mdb/dbaas-internal-api-image/dbaas_internal_api/apis/search
)

END()

RECURSE(
    config
    console
    schemas
    search
)
